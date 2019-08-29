#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/time.h>

#include "utils.h"
#include "ctlog.h"
#include "ctio.h"
#include "confparser/confparser.h"
#include "confparser/dictionary.h"

#define MAXBUF      4096

#define DEF_LOG_LEVEL   8
#define DEF_RW_TIMEOUT  5

int trans_data();

/************** socket fd ******************/
int pfd_r               = 0;
int pfd_w               = 1;
int client_fd           = 2;
int back_fd             = -1;


/************** CMD Args ******************/
char mid[MAXLINE]       = {0};
char client_addr[MAXLINE]   = {0};
char cip[MAXLINE]       = {0};
char cport[MAXLINE]     = {0};
char chost[MAXLINE]     = {0};
char cfg_ini[MAXLINE]   = {0};


/************** config items ******************/
char clog_level[128]    = {0};
char crw_timeout[128]   = {0};

char cback_ip[MAXLINE]  = {0};
char cback_port[MAXLINE]    = {0};


/************** global var ******************/
char iobuf[MAXBUF]      = {0};
dictionary *dict_conf   = NULL;



/**
 * @param cfg_ini   配置文件
 * @return 0:succ 1:fail
 */
int get_config_file(char *cfg_ini)
{
    dict_conf = open_conf_file(cfg_ini);
    if (dict_conf == NULL) {
        log_error("open config file[%s] fail", cfg_ini);
        return 1;
    }

    // Log Level
    char *plog_level = dictionary_get(dict_conf, "child:log_level", NULL);
    if (plog_level == NULL) {
        log_warning("parse config 'log_level' fail, use default:%s", DEF_LOG_LEVEL);
        snprintf(clog_level, sizeof(clog_level), "%s", DEF_LOG_LEVEL);
    } else {
        snprintf(clog_level, sizeof(clog_level), "%s", plog_level);
    }

    // RW timeout
    char *prw_timeout = dictionary_get(dict_conf, "child:rw_timeout", NULL);
    if (prw_timeout == NULL) {
        log_warning("parse config 'rw_timeout' fail, use default:%s", DEF_RW_TIMEOUT);
        snprintf(crw_timeout, sizeof(crw_timeout), "%s", DEF_RW_TIMEOUT);
    } else {
        snprintf(crw_timeout, sizeof(crw_timeout), "%s", prw_timeout);
    }

    // BACK IP
    char *pback_ip = dictionary_get(dict_conf, "child:back_ip", NULL);
    if (pback_ip == NULL) {
        log_error("parse config 'back_ip' fail, you must set mysql info");
        return 1;
    } else {
        snprintf(cback_ip, sizeof(cback_ip), "%s", pback_ip);
    }

    // BACK PORT
    char *pback_port = dictionary_get(dict_conf, "child:back_port", NULL);
    if (pback_port == NULL) {
        log_error("parse config 'db_port' fail, you must set mysql info");
        return 1;
    } else {
        snprintf(cback_port, sizeof(cback_port), "%s", pback_port);
    }

    return 0;
}



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int connect_to_back_server(char *back_ip, char *back_port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    char b_host[NI_MAXHOST];
    char b_port[NI_MAXSERV];
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(back_ip, back_port, &hints, &servinfo)) != 0) {
        log_info("getaddrinfo: %s:%s fail:%s", back_ip, back_port, gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if (rv = getnameinfo(p->ai_addr, p->ai_addrlen,
                            b_host, sizeof(b_host),
                            b_port, sizeof(b_port), NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
            log_info("getnameinfo fail: %s", gai_strerror(rv));
        }

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log_info("create socket for remote(%s:%s) fail:%s", b_host, b_port, strerror(errno));
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            log_info("connect remote(%s:%s) fail:%s", b_host, b_port, strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL) {
        log_info("connect remote(%s:%s) fail:%s", b_host, b_port, strerror(errno));
        return -1;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    log_info("connect to remote(%s) succ", s);

    freeaddrinfo(servinfo);

    return sockfd;

}




void close_all_fd()
{
    close(client_fd);
    client_fd = -1;
    close(pfd_r);
    pfd_r = -1;
    close(pfd_w);
    pfd_w = -1;

    if (back_fd != -1) {
        close(back_fd);
        back_fd = -1;
    }
}


void exit_cleanup(int errnum)
{
    log_info("exit");
    close_all_fd();
    _exit(errnum);
}




void usage(char *prog)
{
    printf("Usage:\n");
    printf("  %s -i[session id] -r[ip:port] -c[config file]\n");
}

/**
 * @cmd ./lp_process -i[session id] -r[ip:port]
 *
 * @fd
 *  0 -> pipe fd read from parent 
 *  1 -> pipe fd write to parent 
 *  2 -> client fd
 */
int main(int argc, char **argv)
{
    char wbuf[MAXLINE] = {0};
    int nw;
    int ch;
    const char *args = "i:r:c:h";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'i':
                snprintf(mid, sizeof(mid), "%s", optarg);
                break;
            case 'r':
                snprintf(client_addr, sizeof(client_addr), "%s", optarg);
                break;
            case 'c':
                snprintf(cfg_ini, sizeof(cfg_ini), "%s", optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit_cleanup(0);
        }
    }

    if (get_config_file(cfg_ini) == 1) {
        exit_cleanup(50);
    }

    // start log process
    ctlog_level = atoi(clog_level);
    ctlog_open("rp_child", LOG_PID|LOG_NDELAY, LOG_MAIL);
    snprintf(ctlog_sid, sizeof(ctlog_sid), "%s", mid);

    log_info("start process ...");

    char *ptok = (char *)memchr(client_addr, ':', strlen(client_addr));
    if (ptok) {
        *ptok = '\0';
        snprintf(cip, sizeof(cip), "%s", client_addr);
        snprintf(cport, sizeof(cport), "%s", ptok+1);
        *ptok = ':';
    }

    if (ndelay_on(client_fd) == -1) {
        log_error("set Client fd(%d) nonblock fail(%d):%s", client_fd, errno, strerror(errno));

        nw = snprintf(wbuf, sizeof(wbuf), "@ERROR: SYSTEM ERROR 1011.1\n");
        nw = safe_write(client_fd, wbuf, nw, atoi(crw_timeout));
        log_info("write buf(%d)(%s) to client fd(%d)", nw, wbuf, client_fd);
        exit_cleanup(1011);
    }

    /*********************************************
     *  connect to back server
     ********************************************/
    back_fd = connect_to_back_server(cback_ip, cback_port);
    if (back_fd < 0) {
        log_error("connect to back server(%s:%s) fail", cback_ip, cback_port);
        nw = snprintf(wbuf, sizeof(wbuf), "@ERROR: SYSTEM ERROR 1010\n");
        nw = safe_write(client_fd, wbuf, nw, atoi(crw_timeout));
        log_info("write buf(%d)(%s) to client fd(%d)", nw, wbuf, client_fd);
        exit_cleanup(1010);
    }
    log_info("connect to back server(%s:%s) succ. fd(%d)", cback_ip, cback_port, back_fd);

    if (ndelay_on(back_fd) == -1) {
        log_error("set back fd[%d] nonblock fail:[%d]%s", back_fd, errno, strerror(errno));
        nw = snprintf(wbuf, sizeof(wbuf), "@ERROR: SYSTEM ERROR 1011\n");
        nw = safe_write(client_fd, wbuf, nw, atoi(crw_timeout));
        log_info("write buf(%d)(%s) to client fd(%d)", nw, wbuf, client_fd);
        exit_cleanup(1011);
    }


    /********* Transfer Data ********************/
    int retval = trans_data();
    if (retval < 0) {
        log_error("transfer data fail");
    }

    log_info("transfer data finish");

    exit_cleanup(1);
}



int trans_data()
{
    fd_set rfds;
    struct timeval tv;
    int retval;
    int n;

    int maxfd = back_fd > client_fd ? back_fd : client_fd;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(back_fd, &rfds);
        FD_SET(client_fd, &rfds);

        tv.tv_sec = atoi(crw_timeout);
        tv.tv_usec = 0;

        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (retval == 0) {
            continue;
        } else if (retval == -1) {
            log_error("call select fail(%d):%s", errno, strerror(errno));
            return -1;
        }

        if (FD_ISSET(back_fd, &rfds)) {
            bzero(iobuf, MAXBUF);
            n = read(back_fd, iobuf, MAXBUF);
            if(n == 0) {
                log_info("Read Back (%s:%s) disconnect.", cback_ip, cback_port);
                return 0;
            } else if (n == -1) {
                log_error("Read Back (%s:%s) len(%d) fail(%d):%s",
                    cback_ip, cback_port, n, errno, strerror(errno));
                return -1;
            }

            n = safe_write(client_fd, iobuf, n, atoi(crw_timeout));
            if (n < 0) {
                log_error("Write Client (%s) len(%d) fail(%d):%s", client_addr, n, errno, strerror(errno));
                return -1;
            }
        }

        if (FD_ISSET(client_fd, &rfds)) {
            bzero(iobuf, MAXBUF);
            n = read(client_fd, iobuf, MAXBUF);
            if (n == 0) {
                log_info("Read Client (%s) disconnect", client_addr);
                return 0;
            } else if (n == -1) {
                log_error("Read Client (%s) len(%d) fail(%d):%s", client_addr, n, errno, strerror(errno));
                return -1;
            }

            n = safe_write(back_fd, iobuf, n, atoi(crw_timeout));
            if (n < 0) {
                log_info("Write Back (%s:%s) len(%d) fail(%d):%s",
                            cback_ip, cback_port, n, errno, strerror(errno));
                return -1;
            }
        }

    }

    return 0;
}


