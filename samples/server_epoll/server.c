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

#include "server.h"
#include "utils.h"

#include "ctlog/ctlog.h"
#include "confparser/confparser.h"
#include "confparser/dictionary.h"



// ------ CMD param ------
char cfg_ini[MAX_LINE] = {0};
char cport[128]         = {0};
char cmax_connect[128]  = {0};
char clog_level[128]    = {0};
char crw_timeout[128]   = {0};

// ------ Epoll ------
int epoll_fd            = -1;
int epoll_nfds          = -1;
int epoll_event_num     = 0;
int epoll_num_running   = 0;
struct epoll_event *epoll_evts = NULL;

// ------ Socket ------
int listen_fd           = -1;

// ------ Client ------
struct client_st *clients_t = NULL;





void sigchld_exit()
{
    int exitcode;

    int wstat, pid;
    while ((pid = waitpid(-1, &wstat, WNOHANG)) > 0) {
        //log_info("child exit[%d]", wstat);

        if (wait_crashed(wstat)) {
            //log_error("child crashed[%d]", wstat);
            return;
        }

        exitcode = wait_exitcode(wstat);
        //log_info("child exitcode[%d]", exitcode);
    }
}


// ------ Client List ------
/**
 * 初始化client
 */
void init_client_with_idx(int i)
{
    clients_t[i].used   = 0;
    clients_t[i].pid    = -1;
    clients_t[i].fd     = -1;
    clients_t[i].pfd_r  = -1;
    clients_t[i].pfd_w  = -1;

    memset(clients_t[i].ip, 0, sizeof(clients_t[i].ip));
    memset(clients_t[i].port, 0, sizeof(clients_t[i].port));
    memset(clients_t[i].sid, 0, sizeof(clients_t[i].sid));
}

void clean_client_with_idx(int i)
{
    clients_t[i].used   = 0;
    clients_t[i].pid    = -1;

    if (clients_t[i].fd != -1) {
        close(clients_t[i].fd);
        clients_t[i].fd = -1;
    }

    if (clients_t[i].pfd_r != -1) {
        close(clients_t[i].pfd_r);
        clients_t[i].pfd_r = -1;
    }

    if (clients_t[i].pfd_w != -1) {
        close(clients_t[i].pfd_w);
        clients_t[i].pfd_w = -1;
    }

    memset(clients_t[i].ip, 0, sizeof(clients_t[i].ip));
    memset(clients_t[i].port, 0, sizeof(clients_t[i].port));
    memset(clients_t[i].sid, 0, sizeof(clients_t[i].sid));
}


/**
 * 获取一个空闲的client list索引
 *
 * @return -1:失败 >=0:空闲的索引
 */
int get_idle_idx_from_clients()
{
    int idx = -1;
    int i   = 0;
    for (i = 0; i<atoi(cmax_connect); i++) {
        if (clients_t[i].used == 0) {
            idx = i;
            break;
        }
    }
    return idx;
}

int get_idx_with_sockfd(int sockfd)
{
    int idx = -1;
    int i   = 0;
    for (i=0; i<atoi(cmax_connect); i++) {
        log_debug("clients_t[%d] pfd_r[%d] used[%d] sockfd[%d]", i, clients_t[i].pfd_r, clients_t[i].used, sockfd);
        if ((clients_t[i].pfd_r == sockfd)
            && (clients_t[i].used == 1)) {
            idx = i;
            break;
        }
    }
    return idx;
}


/*
 * @return
 *     -9: system error
 */
int create_child_and_exec_with_idx_clientfd(int i, int connfd)
{
    /**
     *    父进程          子进程
     *
     *     p1[1] ------->  p1[0]
     *     p2[0] <-------  p2[1]
     *
     */
    int pfd1[2], pfd2[2];
    if (pipe(pfd1) == -1) {
        log_error("unable to create pipe:[%d]%s", errno, strerror(errno));

        return -9;

    }
    if (pipe(pfd2) == -1) {
        log_error("unable to create pipe:[%d]%s", errno, strerror(errno));

        close(pfd1[0]);
        close(pfd1[1]);
        pfd1[0] = -1;
        pfd1[1] = -1;

        return -9;
    }
    log_debug("create pfd1[0]:%d pfd1[1]:%d", pfd1[0], pfd1[1]);
    log_debug("create pfd2[0]:%d pfd2[1]:%d", pfd2[0], pfd2[1]);

    // Create Unique ID
    n = create_unique_id(clients_t[i].sid, sizeof(clients_t[i].sid));
    if (n != 16) {
        log_error("create unique id fail");

        close(pfd1[0]);
        close(pfd1[1]);
        close(pfd2[0]);
        close(pfd2[1]);
        pfd1[0] = -1;
        pfd1[1] = -1;
        pfd2[0] = -1;
        pfd2[1] = -1;

        return -9;
    }
    log_debug("create unique id:%s", clients_t[i].sid);

    // 当程序执行exec函数时本fd将被系统自动关闭,表示不传递给exec创建的新进程
    fcntl(pfd1[1], F_SETFD, FD_CLOEXEC);
    fcntl(pfd2[0], F_SETFD, FD_CLOEXEC);
    fcntl(listen_fd, F_SETFD, FD_CLOEXEC);

    int pid = fork();
    if (pid < 0) {
        log_error("fork fail:[%d]%s", errno, strerror(errno));

        close(pfd1[0]);
        close(pfd1[1]);
        close(pfd2[0]);
        close(pfd2[1]);
        pfd1[0] = -1;
        pfd1[1] = -1;
        pfd2[0] = -1;
        pfd2[1] = -1;

        return -9;

    } else if (pid == 0) {
        // 子进程
        close(pfd1[1]);
        close(pfd2[0]);
        pfd1[1] = -1;
        pfd2[0] = -1;

        close(listen_fd);
        listen_fd = -1;

        if (fd_move(2, connfd) == -1) {
            log_error("%s fd_move(2, %d) fail:[%d]%s", clients_t[i].sid, connfd, errno, strerror(errno));
            _exit(50);
        }

        if (fd_move(0, pfd1[0]) == -1) {
            log_error("%s fd_move(0, %d) fail:[%d]%s", clients_t[i].sid, pfd1[0], errno, strerror(errno));
            _exit(50);
        }

        if (fd_move(1, pfd2[1]) == -1) {
            log_error("%s fd_move(1, %d) fail:[%d]%s", clients_t[i].sid, pfd2[1], errno, strerror(errno));
            _exit(50);
        }

        // launch child program
        char child_sid[MAX_LINE] = {0};
        char child_remote[MAX_LINE] = {0};
        char child_cfg[MAX_LINE] = {0};
        snprintf(child_sid, sizeof(child_sid), "-i%s", clients_t[i].sid);
        snprintf(child_remote, sizeof(child_remote), "-r%s:%s", clients_t[i].ip, clients_t[i].port);
        snprintf(child_cfg, sizeof(child_cfg), "-c%s", cfg_ini);

        char *args[5];
        args[0] = CHILD_PROG;
        args[1] = child_sid;
        args[2] = child_remote;
        args[3] = child_cfg;
        args[4] = 0;

        char exec_log[MAX_LINE * 3] = {0};
        char *pexec_log = exec_log;
        int len = 0;
        int i = 0;
        while (args[i] != 0) {
            nw = snprintf(pexec_log + len, sizeof(exec_log) - len, "%s ", args[i]);
            len += nw;
            i++;
        }

        log_info("Exec:[%s]", exec_log);

        if (execvp(*args, args) == -1) {
            log_error("execvp fail:[%d]%s", errno, strerror(errno));
            _exit(50);
        }

        _exit(50);
    }

    // 父进程
    clients_t[i].pid = pid;

    close(pfd1[0]);
    close(pfd2[1]);
    pfd1[0] = -1;
    pfd2[1] = -1;

    close(connfd);
    connfd = -1;

    clients_t[i].pfd_r = pfd2[0];
    clients_t[i].pfd_w = pfd1[1];

    log_debug("clients_t[%d] pfd_r[%d] pfd_w[%d]", i, pfd2[0], pfd1[1]);

    if (ndelay_on(clients_t[i].pfd_r) == -1) {
        log_error("set noblocking fd[%d] fail:[%d]%s", clients_t[i].pfd_r, errno, strerror(errno));
    }

    struct epoll_event pipe_r_ev;
    pipe_r_ev.events = EPOLLIN | EPOLLET;
    pipe_r_ev.data.fd = clients_t[i].pfd_r;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_r_ev.data.fd, &pipe_r_ev) == -1) {
        log_error("epoll_ctl client fd[%d] fail:[%d]%s", pipe_r_ev.data.fd, errno, strerror(errno));
    }
    log_debug("epoll_add fd[%d]", pipe_r_ev.data.fd);

    return 0;
}



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
    char *plog_level = dictionary_get(dict_conf, "server:log_level", NULL);
    if (plog_level == NULL) {
        log_warning("parse config 'log_level' fail, use default:%s", DEF_LOG_LEVEL);
        snprintf(clog_level, sizeof(clog_level), "%s", DEF_LOG_LEVEL);
    } else {
        snprintf(clog_level, sizeof(clog_level), "%s", plog_level);
    }

    // Bind Port
    char *pbind_port = dictionary_get(dict_conf, "server:bind_port", NULL);
    if (pbind_port == NULL) {
        log_warning("parse config 'bind_port' fail, use default:%s", DEF_PORT);
        snprintf(cport, sizeof(cport), "%s", DEF_PORT);
    } else {
        snprintf(cport, sizeof(cport), "%s", pbind_port);
    }

    // Max Connect
    char *pmax_connect = dictionary_get(dict_conf, "server:max_connect", NULL);
    if (pmax_connect == NULL) {
        log_warning("parse config 'max_connect' fail, use default:%s", DEF_MAX_WORKS);
        snprintf(cmax_connect, sizeof(cmax_connect), "%s", DEF_MAX_WORKS);
    } else {
        snprintf(cmax_connect, sizeof(cmax_connect), "%s", pmax_connect);
    }

    // RW Timeout
    char *prw_timeout = dictionary_get(dict_conf, "server:rw_timeout", NULL);
    if (prw_timeout == NULL) {
        log_warning("parse config 'rw_timeout' fail, use default:%s", DEF_RW_TIMEOUT);
        snprintf(crw_timeout, sizeof(crw_timeout), "%s", DEF_RW_TIMEOUT);
    } else {
        snprintf(crw_timeout, sizeof(crw_timeout), "%s", prw_timeout);
    }

    /*
    if (dict_conf != NULL) {
        dictionary_del(dict_conf);
        dict_conf = NULL;
    }*/

    return 0;
}



void usage(char *prog)
{
    printf("Usage:\n");
    printf("  %s -c[config file]\n", prog);
}


int main(int argc, char **argv)
{
    int i = 0;

    int ch;
    const char *args = "c:h";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'c':
                snprintf(cfg_ini, sizeof(cfg_ini), "%s", optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
        }
    }

    if (get_config_file(cfg_ini) == 1) {
        exit(0);
    }

    // start log process
    ctlog_level = atoi(clog_level);
    ctlog("server", LOG_PID|LOG_NDELAY, LOG_MAIL);


    // client
    clients_t = (struct client_st *)malloc((atoi(cmax_connect) + 1) * sizeof(struct client_st));
    if (clients_t == NULL) {
        log_error("malloc clients [%d] fail:[%d]%s", (atoi(cmax_connect) + 1), errno, strerror(errno));
        exit(1);
    }

    // init clients
    for (i=0; i<(atoi(cmax_connect) + 1); i++) {
        init_client_with_idx(i);
    }


    // ------ Socket ------
    int connfd, epfd, sockfd, n, nr, nw;
    struct sockaddr_in local, remote;
    socklen_t addrlen;

    // Create listen socket
    int bind_port = atoi(cport);
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_error("socket fail:[%d]%s", errno, strerror(errno));
        exit(1);
    }

    // 设置套接字选项避免地址使用错误,解决: Address already in use
    int on = 1;
    if ((setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
        log_error("set socket REUSEADDR fail:[%d]%s", errno, strerror(errno));
        exit(1);
    }

    bzero(&local, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(bind_port);

    // Bind port
    if (bind(listen_fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        log_error("bind local port:%d fail:[%d]%s", bind_port, errno, strerror(errno));
        exit(1);
    }
    log_debug("bind local port %d succ", bind_port);

    // 设置listen_fd为非阻塞
    if (ndelay_on(listen_fd) == -1) {
        log_error("set noblocking fd[%d] fail:[%d]%s", listen_fd, errno, strerror(errno));
        exit(1);    
    }

    // Listen
    if (listen(listen_fd, atoi(cmax_connect)) != 0) {
        log_error("listen fd[%d] max number[%d] fail:[%d]%s", listen_fd, atoi(cmax_connect), errno, strerror(errno));
        exit(1);
    }

    // Ignore pipe signal
    sig_pipeignore();
    // Catch signal which is child program exit
    sig_catch(SIGCHLD, sigchld_exit);


    // ------ Epoll ------
    epoll_event_num = atoi(cmax_connect) + 1;
    epoll_evts      = NULL;
    epoll_fd        = -1;
    epoll_nfds      = -1;

    int epoll_i     = 0;
    // 创建事件数组并清零
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("malloc epoll events [%d] fail:[%d]%s", (epoll_event_num * sizeof(struct epoll_event)), errno, strerror(errno));
        exit(1);
    }

    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd == -1) {
        log_error("epoll create max number %d fail:[%d]%s", epoll_event_num, errno, strerror(errno));
        exit(1);
    }

    // 设置ET模式
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        log_error("epoll_ctl: listen socket fail:[%d]%s", errno, strerror(errno));
        exit(1);
    }

    epoll_num_running = 0;

    for (;;) {

        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, -1);
        log_debug("epoll running number:%d nfds:%d", epoll_num_running, epoll_nfds);

        for (epoll_i = 0; epoll_i < epoll_nfds; epoll_i++) {
            //sig_childblock();

            int evt_fd = epoll_evts[epoll_i].data.fd;
            int evt    = epoll_evts[epoll_i].events;

            if (evt & EPOLLERR) {
                // ------ 监控到错误 ------
                log_error("epoll error:[%d]%s", errno, strerror(errno));
                close(evt_fd);
                continue;

            } else if (evt & EPOLLIN) {
                // ------ 监控到可读 ------
                if (evt_fd == listen_fd) {  
                    // ------ 处理新接入的socket ------
                    while (1) {
                        connfd = accept(listen_fd, (struct sockaddr *)&remote, &addrlen);
                        if (connfd == -1) {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                                // 资源暂时不可读，再来一遍
                                break;
                            } else {
                                log_error("accept error");
                                break;
                            }
                        }

                        // Get a new index from client lists
                        int i = get_idle_idx_from_clients();
                        if (i == -1) {
                            log_error("get idle index from client list fail: maybe client queue is full.");
                            close(connfd);
                            continue;
                        }
                        clients_t[i].used = 1;
                        clients_t[i].fd = connfd;

                        // Get client IP and Port
                        char *ipaddr = inet_ntoa(remote.sin_addr);
                        struct sockaddr_in sa;
                        int len = sizeof(sa);
                        if (getpeername(connfd, (struct sockaddr *)&sa, &len)) {
                            log_error("get client ip and port fail:[%d]%s", errno, strerror(errno));
                        }
                        snprintf(clients_t[i].ip, sizeof(clients_t[i].ip), "%s", inet_ntoa(sa.sin_addr));
                        snprintf(clients_t[i].port, sizeof(clients_t[i].port), "%d", ntohs(sa.sin_port));

                        // Fork child and exec
                        int ret = create_child_and_exec_with_idx_clientfd(i, connfd);
                        if (ret != 0) {
                            char error_buf[] = "Server Temporarily Error, Please Try Again Later\n";
                            write_fd_timeout(connfd, error_buf, strlen(error_buf), atoi(crw_timeout));

                            close(connfd);
                            continue;
                        }

                        epoll_num_running++;
                    }
                    // continue

                } else {
                    // ------ 处理子进程可读数据 ------

                    // Get child sid
                    char child_mid[MAX_LINE] = {0};
                    int idx = get_idx_with_sockfd(evt_fd);
                    if (idx < 0 ) {
                        log_error("get child index with socket fd:%d fail", evt_fd);
                        snprintf(child_mid, sizeof(child_mid), "00000000");
                    } else {
                        log_debug("get_idx_with_sockfd(%d) idx[%d]", evt_fd, idx);
                        snprintf(child_mid, sizeof(child_mid), "%s", clients_t[idx].sid);
                    }

                    while (1) {
                        ssize_t nr;
                        char buf[512] = {0};
                        nr = read(evt_fd, buf, sizeof(buf));
                        if (nr == -1) {         // 循环读完所有数据，结束 
                            if (errno != EAGAIN) {
                                log_error("%s read data from child fail:[%d]%s", child_mid, errno, strerror(errno));
                                close(evt_fd);
                                break;
                            }
                    
                            log_debug("%s finished to read all data from child", child_mid);
                            break;

                        } else if (nr == 0) {   // fd主动关闭请求
                            log_info("%s Closed connection on descriptor %d", child_mid, evt_fd);
                            close(evt_fd);
                            break;

                        }

                        // 打印获取的数据
                        log_info("%s read from fd:%d buf:[%d]%s", child_mid, evt_fd, nr, buf);

                    }
                }

            } else if ((evt & EPOLLHUP) && (evt_fd != listen_fd)) {
                // ------ 有子进程退出 ------
                
                int idx = get_idx_with_sockfd(evt_fd);
                if (idx < 0) {
                    log_error("00000000 get index with socket fd[%d] fail, so not process", evt_fd);
                    continue;
                }
                log_debug("%s get event EPOLLHUP: epoll_i[%d] fd[%d] get fd[%d], used[%d]", clients_t[idx].sid, epoll_i,
                        epoll_evts[epoll_i].data.fd, clients_t[idx].pfd_r, clients_t[idx].used);

                // 子进程清理
                clean_client_with_idx(idx);

                close(evt_fd);

                epoll_num_running--;

                continue;
            }

            //sig_childunblock();
        }
    }

    close(epoll_fd);
    close(listen_fd);
    epoll_fd = -1;
    listen_fd = -1;

    if (clients_t != NULL) {
        free(clients_t);
        clients_t = NULL;
    }
    if (epoll_evts != NULL) {
        free(epoll_evts);
        epoll_evts = NULL;
    }

    log_info("I'm finish, byebye.");

    return 0;
}


