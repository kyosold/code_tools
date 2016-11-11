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

#include "utils.h"
#include "child.h"

#include "ctlog/ctlog.h"
#include "confparser/confparser.h"
#include "confparser/dictionary.h"



int pfd_r               = 0;
int pfd_w               = 1;
int client_fd           = 2;

// ------ Epoll ------
int epoll_fd            = -1;
int epoll_nfds          = -1;
int epoll_event_num     = 0;
struct epoll_event *epoll_evts = NULL;

// ------ CMD param ------
char mid[MAX_LINE]      = {0};
char remote[MAX_LINE]   = {0};
char cfg_ini[MAX_LINE]  = {0};

char clog_level[128]            = {0};
char crw_timeout[128]           = {0};

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

    return 0;
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
        _exit(50);
    }

    // start log process
    ctlog_level = atoi(clog_level);
    ctlog("child", LOG_PID|LOG_NDELAY, LOG_MAIL);
    snprintf(ctlog_sid, sizeof(ctlog_sid), "%s", mid);

    log_info("start process");

    // Epoll Initialize
    int epoll_i = 0;
    epoll_event_num = 4;
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("alloc epoll event fail:[%d]%s", errno, strerror(errno));
        _exit(50);
    }

    // Epoll Create FD
    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd <= 0) {
        log_error("create epoll fd failed:[%d]%s", errno, strerror(errno));
        _exit(50);
    }

    // ------ Add client fd to epoll
    if (ndelay_on(client_fd) == -1) {
        log_error("set fd[%d] nonblock fail:[%d]%s", client_fd, errno, strerror(errno));
        _exit(50);
    }

    struct epoll_event client_evt;
    client_evt.events = EPOLLIN | EPOLLET;
    client_evt.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_evt) == -1) {
        log_error("add event to epoll fail. fd:%d event:EPOLLIN", client_evt.data.fd);
        _exit(50);
    }
    log_debug("add event to epoll fail. fd:%d event:EPOLLIN", client_evt.data.fd);

    // ------ Add pfd_r fd to epoll
    if (ndelay_on(pfd_r) == -1) {
        log_error("set fd[%d] nonblock fail:[%d]%s", pfd_r, errno, strerror(errno));
        _exit(50);
    }

    struct epoll_event pfd_r_evt;
    pfd_r_evt.events = EPOLLIN | EPOLLET;
    pfd_r_evt.data.fd = pfd_r;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pfd_r, &pfd_r_evt) == -1) {
        log_info("add event to epoll fail. fd:%d event:EPOLLIN", pfd_r_evt.data.fd);
        _exit(50);
    }
    log_debug("add parent pipe read fd:%d event to epoll succ", pfd_r_evt.data.fd);


    int rw_timeout = atoi(crw_timeout) * 1000;
    for (;;) {
        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, rw_timeout);
        /*if (epoll_nfds == -1) {
            if (errno == EINTR) {
                log_info("epoll_wait recive EINTR signal, continue");
                continue;
            }

            _exit(50);
        } else if (epoll_nfds == 0) {
            log_info("epoll_wait client timeout[%d s], exit", atoi(crw_timeout));

            _exit(50);
        }*/

        for (epoll_i=0; epoll_i<epoll_nfds; epoll_i++) {

            int evt_fd = epoll_evts[epoll_i].data.fd;
            int evt    = epoll_evts[epoll_i].events;

            if ((evt & EPOLLIN) && (evt_fd == client_fd)) { // client Read
                log_debug("get event EPOLLIN from client socket fd:%d", evt_fd);

                // ...... Read Process ......
                while (1) {
                    char buf[MAX_LINE] = {0};
                    ssize_t nr = 0;
                    nr = read_fd_timeout(client_fd, buf, sizeof(buf), atoi(crw_timeout));
                    log_debug("read from client:[%d]%s", nr, buf);
                    if (nr == -1) {         // 循环读完所有数据，结束
                        if (errno == EAGAIN) {
                            log_debug("finished to read all data from child");
                            break;
                        }

                        log_error("read data error from client:%s", remote);
                
                        close(client_fd);       
                        close(pfd_r);
                        close(pfd_w);

                        _exit(51);

                    } else if (nr == -2) {  // 读超时
                        log_error("read data timeout from client:%s", remote);

                        close(client_fd);       
                        close(pfd_r);
                        close(pfd_w);

                        _exit(20);

                    } else if (nr == 0) {   // client fd主动关闭请求
                        log_info("%s client:%s quit", mid, remote);

                        close(client_fd);       
                        close(pfd_r);
                        close(pfd_w);

                        _exit(10);
                        
                    }

                    log_info("read data from fd:%d buf:[%d]%s", client_fd, nr, buf);
    
                    // ...... Process ......
                }

            } else if ((evt & EPOLLIN) && (evt_fd == pfd_r)) { // parent Read
                log_debug("get event EPOLLIN from Parent socket fd:%d", evt_fd);

                // 处理父进程数据读取，同上 
                // ...... Process ......

            } else if (evt & EPOLLHUP) {
                log_debug("get event EPOLLHUP socket fd:%d", evt_fd);

                close(client_fd);
                close(pfd_r);
                close(pfd_w);

                _exit(0);
            }

        }

    }


    _exit(1);
}
