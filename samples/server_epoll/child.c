#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/time.h>

#include "utils.h"
#include "child.h"

#include "ctlog.h"
#include "confparser/confparser.h"
#include "confparser/dictionary.h"



int pfd_r               = 0;
int pfd_w               = 1;
int client_fd           = 2;
int back_fd             = -1;


unsigned long long client_buf_read_len = 0;
unsigned long long client_buf_write_len = 0;
unsigned long long back_buf_read_len = 0;
unsigned long long back_buf_write_len = 0;

enum RSYNC_STATE
{
	CONNECT,
    TRANS
};
enum RSYNC_STATE client_state;
enum RSYNC_STATE back_state;

void process_client_cmd(int cfd, char *buf, size_t buf_len);
void process_back_cmd(int bfd, char *buf, size_t buf_len);


// ------ Epoll ------
int epoll_fd            = -1;
int epoll_nfds          = -1;
int epoll_event_num     = 0;
struct epoll_event *epoll_evts = NULL;
struct epoll_event back_evt;

// ------ CMD param ------
char mid[MAX_LINE]      = {0};
char client_addr[MAX_LINE]   = {0};
char remote[MAX_LINE]   = {0};
char cfg_ini[MAX_LINE]  = {0};

char clog_level[128]            = {0};
char crw_timeout[128]           = {0};
char cback_ip[MAX_LINE]			= {0};
char cback_port[MAX_LINE]		= {0};

// ------ Config File Parse ------
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

    // Back ip
    char *pback_ip = dictionary_get(dict_conf, "child:back_ip", NULL);
    if (pback_ip == NULL) {
        log_warning("parse config 'back_ip' fail");
    } else {
        snprintf(cback_ip, sizeof(cback_ip), "%s", pback_ip);
    }

    // Back port
    char *pback_port = dictionary_get(dict_conf, "child:back_port", NULL);
    if (pback_port == NULL) {
        log_warning("parse config 'back_port' fail");
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
    close(pfd_r);
    close(pfd_w);
}


void exit_cleanup(int errnum)
{
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
	int nw;
    int ch;
    const char *args = "i:r:c:h";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'i':
                snprintf(mid, sizeof(mid), "%s", optarg);
                break;
            case 'r':
                snprintf(remote, sizeof(remote), "%s", optarg);
                break;
            case 'c':
                snprintf(cfg_ini, sizeof(cfg_ini), "%s", optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                _exit(0);
        }
    }

    if (get_config_file(cfg_ini) == 1) {
		exit_cleanup(50);
    }

    // start log process
    ctlog_level = atoi(clog_level);
    ctlog("ep_child", LOG_PID|LOG_NDELAY, LOG_MAIL);
    snprintf(ctlog_sid, sizeof(ctlog_sid), "%s", mid);

    log_info("start process");

    // Epoll Initialize
    int epoll_i = 0;
    epoll_event_num = 4;
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("alloc epoll event fail:[%d]%s", errno, strerror(errno));
		exit_cleanup(50);
    }

    // Epoll Create FD
    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd <= 0) {
        log_error("create epoll fd failed:[%d]%s", errno, strerror(errno));
		exit_cleanup(50);
    }

    // ------ Add client fd to epoll
    if (ndelay_on(client_fd) == -1) {
        log_error("set fd[%d] nonblock fail:[%d]%s", client_fd, errno, strerror(errno));
		exit_cleanup(50);
    }

    struct epoll_event client_evt;
    client_evt.events = EPOLLIN | EPOLLET;
    client_evt.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_evt) == -1) {
        log_error("add event to epoll fail. fd:%d event:EPOLLIN", client_evt.data.fd);
		exit_cleanup(50);
    }
    log_debug("add event to epoll fail. fd:%d event:EPOLLIN", client_evt.data.fd);

    // ------ Add pfd_r fd to epoll
    if (ndelay_on(pfd_r) == -1) {
        log_error("set fd[%d] nonblock fail:[%d]%s", pfd_r, errno, strerror(errno));
		exit_cleanup(50);
    }

    struct epoll_event pfd_r_evt;
    pfd_r_evt.events = EPOLLIN | EPOLLET;
    pfd_r_evt.data.fd = pfd_r;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pfd_r, &pfd_r_evt) == -1) {
        log_info("add event to epoll fail. fd:%d event:EPOLLIN", pfd_r_evt.data.fd);
		exit_cleanup(50);
    }
    log_debug("add parent pipe read fd:%d event to epoll succ", pfd_r_evt.data.fd);


    int rw_timeout = atoi(crw_timeout) * 1000;
    for (;;) {
        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, rw_timeout);

        for (epoll_i=0; epoll_i<epoll_nfds; epoll_i++) {

            int evt_fd = epoll_evts[epoll_i].data.fd;
            int evt    = epoll_evts[epoll_i].events;

            if ((evt & EPOLLIN) && (evt_fd == client_fd)) { // client Read
				log_debug("get EVENT EPOLLIN from CLIENT socket fd:%d", evt_fd);

                // ...... Read Process ......
                while (1) {
                    char buf[MAX_LINE] = {0};
                    ssize_t nr = 0;
		
					nr = read(client_fd, buf, sizeof(buf));
					log_debug("READ CLIENT len(%d) errno(%d)", nr, errno);

                    if (nr == -1) {         // 循环读完所有数据，结束
                        if (errno == EAGAIN) {
							log_debug("READ CLIENT data finished");
                            break;
						} else if (errno == EINTR) {
							log_debug("READ CLIENT (%s) data again", client_addr);
							continue;
                        }

						log_error("READ CLIENT (%s) error:%s", client_addr, strerror(errno));
						exit_cleanup(51);

                    } else if (nr == 0) {   // client fd主动关闭请求
						log_info("%s CLIENT (%s) CLOSE", mid, client_addr);
						exit_cleanup(10);
                        
                    }

                    // ...... Process ......
					client_buf_read_len += nr;
					if (client_state == TRANS) {
                        log_info("READ CLIENT fd:%d total(%lld) buf:[%d](%d %d ... %d %d)", client_fd, client_buf_read_len, nr, buf[0], buf[1], buf[nr-1], buf[nr]);

                        if (back_fd != -1) {
                            int nw_tmp = nr;
                            while (nw_tmp > 0) {
                                nw = write(back_fd, buf + nr - nw_tmp, nw_tmp);
                                if (nw < 0) {
                                    if (nw == -1 && errno != EAGAIN) {
                                        log_error("WRITE BACK (%s:%s) error(%d):%s total(%lld) buf:[%d](%d %d ... %d %d)", cback_ip, cback_port, errno, strerror(errno), back_buf_write_len, nw, buf, buf[0], buf[1], buf[nw_tmp-1], buf[nw_tmp]);
                                        exit_cleanup(21);
                                    }
                                    continue;
                                }
                                nw_tmp -= nw;

                                back_buf_write_len += nw;
                            }
                            log_info("WRITE BACK fd:%d total(%lld) buf:[%d](%d %d ... %d %d)", back_fd, back_buf_write_len, nw, buf[0], buf[1], buf[nw-1], buf[nw]);
                        }
					} else {
						log_info("READ CLIENT fd:%d buf:[%d]%s", client_fd, nr, buf);

						char *cmd_line = buf;
						process_client_cmd(client_fd, cmd_line, strlen(cmd_line));
					}
                }

			} else if ((evt & EPOLLIN) && (evt_fd == back_fd)) { // Back Read
				log_debug("get EVENT EPOLLIN from BACK socket fd:%d", evt_fd);

				// ...... Read Process ......
				while (1) {
					char buf[MAX_LINE] = {0};
                    ssize_t nr = 0;

					nr = read(back_fd, buf, sizeof(buf));
                    log_debug("READ BACK len(%d) errno(%d)", nr, errno);
					
					if (nr == -1) {         // 循环读完所有数据，结束
                        if (errno == EAGAIN) {
                            log_debug("READ BACK data finished");
                            break;
                        } else if (errno == EINTR) {
                            log_debug("READ BACK (%s:%s) data AGAIN", cback_ip, cback_port);
                            continue;
                        }
                        log_error("READ BACK (%s:%s) error(%d):%s", cback_ip, cback_port, errno, strerror(errno));
                        exit_cleanup(51);

					} else if (nr == 0) {   // client fd主动关闭请求
                        log_info("%s BACK (%s:%s) CLOSE", mid, cback_ip, cback_port);
                        exit_cleanup(10);
                    }

					// ...... Process ......
                    back_buf_read_len += nr;
                    if (back_state == TRANS) {
                        log_info("READ BACK fd:%d total(%lld) buf:[%d](%d %d ... %d %d)", back_fd, back_buf_read_len, nr, buf[0], buf[1], buf[nr-1], buf[nr]);

                        int nw_tmp = nr;
                        while (nw_tmp > 0) {
                            nw = write(client_fd, buf + nr - nw_tmp, nw_tmp);
                            if (nw <0) {
                                if (nw == -1 && errno != EAGAIN) {
                                    log_error("WRITE CLIENT fd:%s error(%d):%s total(%lld) buf:[%d](%d %d ... %d %d)", client_fd, errno, strerror(errno), client_buf_write_len, nw, buf[0], buf[1], buf[nw-1], buf[nw]);
                                    exit_cleanup(21);
                                }
                            }
                            nw_tmp -= nw;
                            client_buf_write_len += nw;
                        }
                        log_info("WRITE CLIENT fd:%d total(%lld) buf:[%d](%d %d ... %d %d)", client_fd, client_buf_write_len, nw, buf[0], buf[1], buf[nw-1], buf[nw]);

                    } else {
                        log_info("READ BACK fd:%d buf:[%d]%s", back_fd, nr, buf);

						char *cmd_line = buf;
						process_back_cmd(back_fd, cmd_line, strlen(cmd_line));
					}	
				}

            } else if ((evt & EPOLLIN) && (evt_fd == pfd_r)) { // parent Read
                log_debug("get event EPOLLIN from Parent socket fd:%d", evt_fd);

                // 处理父进程数据读取，同上 
                // ...... Process ......

            } else if (evt & EPOLLHUP) {
                log_debug("get event EPOLLHUP socket fd:%d", evt_fd);

				exit_cleanup(0);
            }

        }

    }

	exit_cleanup(1);
}


void process_client_cmd(int cfd, char *buf, size_t buf_len)
{
    log_info("process client state(%d) command(%s)", client_state, buf);

	int nw = 0;
    char wbuf[MAX_LINE] = {0};

	if (client_state == CONNECT && *cback_ip != '\0' && *cback_port != '\0') {
		/*******************************************/
        /*      connect back server          */
        /*******************************************/
        back_fd = connect_to_back_server(cback_ip, cback_port);
        if (back_fd < 0) {
            log_error("connect to back server(%s:%s) fail", cback_ip, cback_port);

            nw = snprintf(wbuf, sizeof(wbuf), "@ERROR: SYSTEM ERROR 1010\n");
            nw = write(cfd, wbuf, nw);
            log_info("write buf(%d)(%s) to client fd(%d)", nw, wbuf, cfd);

            exit_cleanup(1010);
        }
        log_info("connect to back server(%s:%s) succ. fd(%d)", cback_ip, cback_port, back_fd);
	
		// ------ Add back fd to epoll
        if (ndelay_on(back_fd) == -1) {
            log_error("set back fd[%d] nonblock fail:[%d]%s", back_fd, errno, strerror(errno));

            nw = snprintf(wbuf, sizeof(wbuf), "@ERROR: SYSTEM ERROR 1011\n");
            nw = write(cfd, wbuf, nw);
            log_info("write buf(%d)(%s) to client fd(%d)", nw, wbuf, cfd);
            exit_cleanup(1011);
        }

        back_evt.events = EPOLLIN | EPOLLET;
        back_evt.data.fd = back_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, back_fd, &back_evt) == -1) {
            log_error("add back event to epoll fail(%s). fd:%d event:EPOLLIN", strerror(errno), back_evt.data.fd);

            nw = snprintf(wbuf, sizeof(wbuf), "@ERROR: SYSTEM ERROR 1012\n");
            nw = write(cfd, wbuf, nw);
            log_info("write buf(%d)(%s) to client fd(%d)", nw, wbuf, cfd);
            exit_cleanup(1012);
        }
        log_debug("add back event to epoll succ. fd:%d event:EPOLLIN", back_evt.data.fd);


		client_state = TRANS;
        back_state = TRANS;

        client_buf_read_len = 0;
        client_buf_write_len = 0;
        back_buf_read_len = 0;
        back_buf_write_len = 0;

        log_info("DATA TRANSA ===================================");

        return;
	}
}


void process_back_cmd(int bfd, char *buf, size_t buf_len)
{
	log_info("process back state(%d) command(%s)", back_state, buf);

}



