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



char mid[MAX_LINE]      = {0};
int pfd_r               = 0;
int pfd_w               = 1;
int client_fd           = 2;

// ------ Epoll ------
int epoll_fd            = -1;
int epoll_nfds          = -1;
int epoll_event_num     = 0;
struct epoll_event *epoll_evts = NULL;

// ------ CMD param ------
char sid[MAX_LINE]      = {0};
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
        _exit(100);
    }

    // start log process
    log_level = atoi(clog_level);
    ctlog("child", LOG_PID|LOG_NDELAY, LOG_MAIL);

    log_info("start process");

    // get UUID
    /*int n = create_unique_id(mid, sizeof(mid));
    if (n != 16) {
        log_error("create unique id fail");

        _exit(100);
    }*/

    // Epoll Initialize
    int epoll_i = 0;
    epoll_event_num = 4;
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("%s alloc epoll event fail:[%d]%s", mid, errno, strerror(errno));
        _exit(100);
    }

    // Epoll Create FD
    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd <= 0) {
        log_error("%s create epoll fd failed:[%d]%s", mid, errno, strerror(errno));
        _exit(100);
    }

    // Add client fd to epoll
    if (ndelay_on(client_fd) == -1) {
        log_error("set fd[%d] nonblock fail:[%d]%s", client_fd, errno, strerror(errno));
        _exit(100);
    }

    struct epoll_event client_evt;
    client_evt.events = EPOLLIN | EPOLLET;
    client_evt.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_evt) == -1) {
        log_error("%s add event to epoll fail. fd:%d event:EPOLLIN", mid, client_evt.data.fd);
        _exit(100);
    }
    log_debug("%s add event to epoll fail. fd:%d event:EPOLLIN", mid, client_evt.data.fd);

    // Add pfd_r fd to epoll
    if (ndelay_on(pfd_r) == -1) {
        log_error("set fd[%d] nonblock fail:[%d]%s", pfd_r, errno, strerror(errno));
        _exit(100);
    }

    struct epoll_event pfd_r_evt;
    pfd_r_evt.events = EPOLLIN | EPOLLET;
    pfd_r_evt.data.fd = pfd_r;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pfd_r, &pfd_r_evt) == -1) {
        log_info("%s add event to epoll fail. fd:%d event:EPOLLIN",  mid, pfd_r_evt.data.fd);
        _exit(100);
    }
    log_debug("%s add parent pipe read fd:%d event to epoll succ", mid, pfd_r_evt.data.fd);


    int rw_timeout = atoi(crw_timeout) * 1000;
    for (;;) {
        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, rw_timeout);
        if (epoll_nfds == -1) {
            if (errno == EINTR) {
                log_info("%s epoll_wait recive EINTR signal, continue", mid);
                continue;
            }

            _exit(100);
        } else if (epoll_nfds == 0) {
            log_info("%s epoll_wait client timeout[%d s], exit", mid, atoi(crw_timeout));

            _exit(100);
        }

        for (epoll_i=0; epoll_i<epoll_nfds; epoll_i++) {
            int evt_fd = epoll_evts[epoll_i].data.fd;
            int evt_event = epoll_evts[epoll_i].events;

            if ((evt_event & EPOLLIN) && (evt_fd == client_fd)) { // client Read
                log_debug("%s get event EPOLLIN from client socket fd:%d", mid, evt_fd);

                // ...... Process ......
				char buf[MAX_LINE] = {0};
				int nr = read_fd_timeout(client_fd, buf, sizeof(buf), atoi(crw_timeout));
				log_info("%s read from client:[%d]%s", mid, nr, buf);
				if (nr == -1) {			// error
					log_error("%s read error from client:%s", mid, remote);
				
					close(client_fd);		
					close(pfd_r);
					close(pfd_w);

					_exit(0);
	
				} else if (nr == 0) {	// client exit
					log_info("%s client:%s quit", mid, remote);

					close(client_fd);		
					close(pfd_r);
					close(pfd_w);

					_exit(0);

				} else {				// process logic
					log_info("%s read buf:[%d]%s from client", mid, nr, buf);

					snprintf(buf, sizeof(buf), "250 OK\n");
					write_fd_timeout(client_fd, buf, strlen(buf), atoi(crw_timeout));
				}

            } else if ((evt_event & EPOLLIN) && (evt_fd == pfd_r)) { // parent Read
                log_debug("%s get event EPOLLIN from Parent socket fd:%d", mid, evt_fd);

                // ...... Process ......
				char buf[MAX_LINE] = {0};
				int nr = read_fd_timeout(pfd_r, buf, sizeof(buf), atoi(crw_timeout));
				log_info("%s read from parent:[%d]%s", mid, nr, buf);
				if (nr == -1 || nr == 0) {	// parent pipe error
					log_error("%s read error from parent pipe", mid);
	
					close(client_fd);
					close(pfd_r);
					close(pfd_w);
		
					_exit(0);
					
				} else {				// Process Logic
					// Process Logic
				}

            } else if (evt_event & EPOLLHUP) {
                log_debug("%s get event EPOLLHUP socket fd:%d", mid, evt_fd);

                close(client_fd);
                close(pfd_r);
                close(pfd_w);

                _exit(0);
            }

        }

    }


    _exit(111);
}
