#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <libgen.h>
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
#include <netdb.h>
#include <getopt.h>

#include "ctserver.h"
#include "utils.h"
#include "ctlog.h"
#include "ctio.h"
#include "confparser/confparser.h"
#include "confparser/dictionary.h"


/************************************
 * ComandLine Args
 ***********************************/
char cfg_ini[MAXLINE]       = {0};
char cport[128]             = {0};
char cmax_connect[128]      = {0};
char clog_level[128]         = {0};
char crw_timeout[128]       = {0};
char cpidfile[MAXLINE]     = {0};
char child_exec[MAXLINE]    = {0};

dictionary *dict_conf       = NULL;


/************************************
 * Epoll And Socket
 ***********************************/
int epoll_fd                = -1;
int epoll_nfds              = -1;
int epoll_event_num         = 0;
int epoll_num_running       = 0;
struct epoll_event *epoll_evts = NULL;

//int listen_fd               = -1;
char b_host[NI_MAXHOST];
char b_port[NI_MAXSERV];
int on = 1;
int listen_socks[128];
int listen_sock_types[128];
int num_listen_socks = 0;

/************************************
 * Global var
 ***********************************/
struct client_st *clients_t = NULL;
int am_daemon = 0;


void sigchld_exit()
{   
    // 此函数内不要做syslog调用，在大并发时会出现系统异常问题
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


void close_fd(int fd)
{
    close(fd);
    fd = -1;
}


/************************************
 * Parse config file
 ***********************************/
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

    // Pid File
    char *ppidfile = dictionary_get(dict_conf, "server:pidfile", NULL);
    if (ppidfile == NULL) {
        log_warning("parse config 'pidfile' fail, use default:%s", DEF_PID_FILE);
        snprintf(cpidfile, sizeof(cpidfile), "%s", DEF_PID_FILE);
    } else {
        snprintf(cpidfile, sizeof(cpidfile), "%s", ppidfile);
    }

    // Child Exec
    char *pchild_exec = dictionary_get(dict_conf, "server:child_exec", NULL);
    if (pchild_exec == NULL) {
        log_warning("parse config 'server:child_exec' fail, use default:%s", DEF_CHILD_EXEC);
        snprintf(child_exec, sizeof(child_exec), "%s", DEF_CHILD_EXEC);
    } else {
        snprintf(child_exec, sizeof(child_exec), "%s", pchild_exec);
    }

    // RW Timeout
    char *prw_timeout = dictionary_get(dict_conf, "server:rw_timeout", NULL);
    if (prw_timeout == NULL) {
        log_warning("parse config 'rw_timeout' fail, use default:%s", DEF_RW_TIMEOUT);
        snprintf(crw_timeout, sizeof(crw_timeout), "%s", DEF_RW_TIMEOUT);
    } else {
        snprintf(crw_timeout, sizeof(crw_timeout), "%s", prw_timeout);
    }

    return 0;
}



static void create_pid_file(void)
{
    char pidbuf[16];
    pid_t pid = getpid();
    int fd, len;

    if (!cpidfile || !*cpidfile)
        return;

    if ((fd = open(cpidfile, O_WRONLY|O_CREAT|O_EXCL, 0666)) == -1) {
      failure:
        fprintf(stderr, "failed to create pid file %s: %s\n", cpidfile, strerror(errno));
        log_error("failed to create pid file %s", cpidfile);
        exit(70);
    }
    snprintf(pidbuf, sizeof pidbuf, "%d\n", (int)pid);
    len = strlen(pidbuf);
    if (write(fd, pidbuf, len) != len)
        goto failure;

    close(fd);

    log_info("create pid file(%s)", cpidfile);
}

/* Become a daemon, discarding the controlling terminal. */
static void become_daemon(int is_daemon)
{
    if (is_daemon == 0) {
        create_pid_file();
        return;
    }

    int i;
    pid_t pid = fork();

    if (pid) {
        if (pid < 0) {
            fprintf(stderr, "failed to fork: %s\n", strerror(errno));
            exit(70);
        }
        _exit(0);
    }

    create_pid_file();

    /* detach from the terminal */
#ifdef HAVE_SETSID
    setsid();
#elif defined TIOCNOTTY
    i = open("/dev/tty", O_RDWR);
    if (i >= 0) {
        ioctl(i, (int)TIOCNOTTY, (char *)0);
        close(i);
    }
#endif
    /* make sure that stdin, stdout an stderr don't stuff things
     * up (library functions, for example) */
    for (i = 0; i < 3; i++) {
        close(i);
        open("/dev/null", O_RDWR);
    }
}


void server_exit()
{
    unlink(cpidfile);

    log_info("/********** I AM QUIT, BYE BYE! *************/");
    exit(1);
}




/************************************
 * Client 
 ************************************/ 
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
    memset(clients_t[i].ip_hex, 0, sizeof(clients_t[i].ip_hex));
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
    memset(clients_t[i].ip_hex, 0, sizeof(clients_t[i].ip_hex));
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
        //log_debug("clients_t[%d] pfd_r[%d] used[%d] sockfd[%d]", i, clients_t[i].pfd_r, clients_t[i].used, sockfd);
        if ((clients_t[i].pfd_r == sockfd)
            && (clients_t[i].used == 1)) {
            idx = i;
            break;
        }
    }
    return idx;
}

/*
 * @return -1:fail other:succ(child list 的索引)
 */
int exit_child_with_sockfd(int sockfd)
{
    int idx = get_idx_with_sockfd(sockfd);
    if (idx < 0) {
        log_error("get index with socket fd[%d] fail, so not process", sockfd);
        close(sockfd);
        return -1;
    }

    // 子进程清理
    clean_client_with_idx(idx);
    close(sockfd);
    return idx;
}



/*
 * @return
 *     -9: system error
 */
int create_child_and_exec_with_idx_clientfd(int i, int listen_fd, int connfd)
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
        log_error("%s unable to create pipe:[%d]%s", clients_t[i].sid, errno, strerror(errno));

        return -9;

    }
    if (pipe(pfd2) == -1) {
        log_error("%s unable to create pipe:[%d]%s", clients_t[i].sid, errno, strerror(errno));

        close(pfd1[0]);
        close(pfd1[1]);
        pfd1[0] = -1;
        pfd1[1] = -1;

        return -9;
    }
    log_debug("%s create pfd1[0]:%d pfd1[1]:%d", clients_t[i].sid, pfd1[0], pfd1[1]);
    log_debug("%s create pfd2[0]:%d pfd2[1]:%d", clients_t[i].sid, pfd2[0], pfd2[1]);

    // 当程序执行exec函数时本fd将被系统自动关闭,表示不传递给exec创建的新进程
    fcntl(pfd1[1], F_SETFD, FD_CLOEXEC);
    fcntl(pfd2[0], F_SETFD, FD_CLOEXEC);
    fcntl(listen_fd, F_SETFD, FD_CLOEXEC);

    int pid = fork();
    if (pid < 0) {
        log_error("%s fork fail:[%d]%s", clients_t[i].sid, errno, strerror(errno));

        close_fd(pfd1[0]);
        close_fd(pfd1[1]);
        close_fd(pfd2[0]);
        close_fd(pfd2[1]);

        return -9;

    } else if (pid == 0) {  // 子进程
        close_fd(pfd1[1]);
        close_fd(pfd2[0]);
        close_fd(listen_fd);

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
        char child_sid[MAXLINE]    = {0};
        char child_remote[MAXLINE] = {0};
        char child_cfg[MAXLINE]    = {0};

        snprintf(child_sid, sizeof(child_sid), "-i%s", clients_t[i].sid);
        snprintf(child_remote, sizeof(child_remote), "-r%s:%s", clients_t[i].ip, clients_t[i].port);
        snprintf(child_cfg, sizeof(child_cfg), "-c%s", cfg_ini);

        char *args[5];
        args[0] = child_exec;
        args[1] = child_sid;
        args[2] = child_remote;
        args[3] = child_cfg;
        args[4] = 0;

        // print exec cmd to log
        char exec_log[MAXLINE * 3] = {0};
        char *pexec_log = exec_log;
        int len = 0, i = 0, nw = 0;
        while (args[i] != 0) {
            nw = snprintf(pexec_log + len, sizeof(exec_log) - len, "%s ", args[i]);
            len += nw;
            i++;
        }
        log_info("Exec:(%s)", exec_log);

        if (execvp(*args, args) == -1) {
            log_error("%s execvp fail:[%d]%s", clients_t[i].sid, errno, strerror(errno));
            _exit(50);
        }

        _exit(50);
    }

    // --------- 父进程 -------------
    clients_t[i].pid = pid;

    close_fd(pfd1[0]);
    close_fd(pfd2[1]);
    close_fd(connfd);

    clients_t[i].pfd_r = pfd2[0];
    clients_t[i].pfd_w = pfd1[1];

    log_debug("%s clients_t[%d] pfd_r[%d] pfd_w[%d]", clients_t[i].sid, i, pfd2[0], pfd1[1]);

    if (ndelay_on(clients_t[i].pfd_r) == -1) {
        log_error("%s set noblocking fd[%d] fail:[%d]%s", clients_t[i].sid, clients_t[i].pfd_r, errno, strerror(errno));
    }


    struct epoll_event pipe_r_ev;
    pipe_r_ev.events = EPOLLIN | EPOLLET;
    pipe_r_ev.data.fd = clients_t[i].pfd_r;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_r_ev.data.fd, &pipe_r_ev) == -1) {
        log_error("%s epoll_ctl client fd[%d] fail:[%d]%s", clients_t[i].sid, pipe_r_ev.data.fd, errno, strerror(errno));
    }
    log_debug("%s epoll_add fd[%d]", clients_t[i].sid, pipe_r_ev.data.fd);

    return 0;
}



int epoll_add_socket(int sockfd, uint32_t events)
{
	struct epoll_event ev;
    ev.events = events;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        log_error("epoll_ctl add: listen socket(%d) fail:[%d]%s", sockfd, errno, strerror(errno));
        return 1;
    }
    return 0;
}


/*
* @return  
*   -1: fail.
*   >0: fd
*/
static int create_and_bind(char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;
    int yes = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family     = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype   = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags      = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo (NULL, port, &hints, &result);
    if (s != 0) {
        log_error("getaddrinfo fail:(%s)", gai_strerror(s));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if ((sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
            log_error("socket fail");
            continue;
        }

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1) {
            close(sfd);
            log_error("bind socket fail(%d):(%s)", errno, strerror(errno));
            continue;
        }

        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            log_error("setsocket SO_REUSEADDR fail(%d):(%s)", errno, strerror(errno));
            return -1;
        }

        break;
    }
    freeaddrinfo(result);

    return sfd;
}

void sock_set_v6only(int fd)
{   
    int on = 1;
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1)
        fprintf(stderr, "setsockopt v6only");
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
int get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in *)sa)->sin_port);
    }
    return (((struct sockaddr_in6 *)sa)->sin6_port);
}


int which_is_listen(int fd)
{
	int i = 0;
	for (i=0; i<num_listen_socks; i++) {
		if (fd == listen_socks[i]) {
			return i;
		}
	}
	return -1;
}



void usage(char *prog)
{
    printf("Usage:\n");
    printf(" %s -c[config file] -d\n", prog);
    printf("\n");
    printf("Options:\n");
    printf(" -c     file full path of config\n");
    printf(" -d     to be a daemon module\n");
    printf(" -h     show this help\n");
    return;
}



/************************************
 * Main
 ************************************/
int main(int argc, char **argv)
{
    int i = 0, ch;
    const char *args = "c:hd";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'c':
                snprintf(cfg_ini, sizeof(cfg_ini), "%s", optarg);
                break;
            case 'd':
                am_daemon = 1;
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (get_config_file(cfg_ini) == 1) {
        exit(0);
    }

    become_daemon(am_daemon);

    // setup log
    ctlog_level = atoi(clog_level);
    ctlog_open(basename(argv[0]), LOG_PID|LOG_NDELAY, LOG_MAIL);

    // client
    clients_t = (struct client_st *)malloc((atoi(cmax_connect) + 1) * sizeof(struct client_st));
    if (clients_t == NULL) {
        log_error("malloc clients [%d] fail:[%d]%s", (atoi(cmax_connect) + 1), errno, strerror(errno));
        server_exit(1);
    }
    log_info("max_connect:%d", atoi(cmax_connect));

    // init clients
    for (i=0; i<(atoi(cmax_connect) + 1); i++) {
        init_client_with_idx(i);
    }

    // socket
    int connfd, n, nr, nw;
	int ipv = 4;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage client_addr;
	socklen_t client_sin_size;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // use mine IP

	if ((rv = getaddrinfo(NULL, cport, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo fail:%s\n", gai_strerror(rv));
        log_error("getaddrinfo fail:%s\n", gai_strerror(rv));
		server_exit(1);
    }

	// find all result in loop, then bind first result
    int tmp_fd = -1;
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if (rv = getnameinfo(p->ai_addr, p->ai_addrlen,
                    b_host, sizeof(b_host),
                    b_port, sizeof(b_port), NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
            log_info("getnameinfo fail:%s", gai_strerror(rv));
            fprintf(stderr, "getnameinfo fail:%s", gai_strerror(rv));
            continue;
        }

        if ((tmp_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log_error("create socket fail(%d):%s", errno, strerror(errno));
            fprintf(stderr, "create socket fail(%d):%s", errno, strerror(errno));
            continue;
        }

		// set nonblock
        if (ndelay_on(tmp_fd) == -1) {
            close(tmp_fd);
            continue;
        }

        // Allow local port reuse in TIME_WAIT.
        if (setsockopt(tmp_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
            log_error("setsockop SO_REUSEADDR fail(%d):%s", errno, strerror(errno));
            fprintf(stderr, "setsockop SO_REUSEADDR fail(%d):%s", errno, strerror(errno));
			server_exit(1);
        }

		if (p->ai_family == AF_INET6) {
            sock_set_v6only(tmp_fd);
            ipv = 6;
        } else if (p->ai_family == AF_INET) {
            ipv = 4;
        }

		if (bind(tmp_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(tmp_fd);
            log_error("bind socket fail(%d):%s", errno, strerror(errno));
            fprintf(stderr, "bind socket fail(%d):%s", errno, strerror(errno));
            continue;
        }

        listen_socks[num_listen_socks] = tmp_fd;
        listen_sock_types[num_listen_socks] = ipv;
        num_listen_socks++;

        if (listen(tmp_fd, atoi(cmax_connect)) == -1) {
            perror("listen");
            exit(1);
        }

        fprintf(stdout, "Server listening on %s port %s\n", b_host, b_port);
        log_info("Server listening on %s port %s\n", b_host, b_port);
	}
	freeaddrinfo(servinfo);


    // Ignore pipe signal
    sig_pipeignore();

    // Catch signal which is child program exit
    sig_catch(SIGCHLD, sigchld_exit);
    // 如果没有特别的事情，可以不用catch子进程退出，
    // 直接忽略子进程退出信号: sig_catch(SIGCHLD, SIG_IGN);

    sig_catch(SIGSEGV, server_exit);
    sig_catch(SIGFPE, server_exit);
    sig_catch(SIGABRT, server_exit);
    sig_catch(SIGBUS, server_exit);

    sig_catch(SIGINT, server_exit);
    sig_catch(SIGHUP, server_exit);
    sig_catch(SIGTERM, server_exit);


    // epoll
    epoll_event_num = atoi(cmax_connect) + 1;
    epoll_evts      = NULL;
    epoll_fd        = -1;
    epoll_nfds      = -1;
    int epoll_i     = 0;

    // 创建事件数组并清零
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("malloc epoll events [%d] fail(%d):(%s)", 
                (epoll_event_num * sizeof(struct epoll_event)), errno, strerror(errno));
        server_exit(1);
    }

    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd == -1) {
        log_error("epoll create max number %d fail(%d):(%s))", epoll_event_num, errno, strerror(errno));
        fprintf(stderr, "epoll create max number %d fail(%d):(%s)", epoll_event_num, errno, strerror(errno));
        server_exit(1);
    }

    // 设置ET模式
	for (epoll_i=0; epoll_i<num_listen_socks; epoll_i++) {
		uint32_t events = EPOLLIN | EPOLLET;
		if (epoll_add_socket(listen_socks[epoll_i], events) != 0) {
			log_error("epoll_ctl: listen fd(%d) socket fail(%d):(%s)", listen_socks[epoll_i], errno, strerror(errno));
			fprintf(stderr, "epoll_ctl: listen fd(%d) socket fail(%d):(%s)", listen_socks[epoll_i], errno, strerror(errno));
			server_exit(1);
		}
	}
	
	epoll_i = 0;
    epoll_num_running = 0;
    for (;;) {
        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, -1);
        log_debug("epoll running number:%d nfds:%d", epoll_num_running, epoll_nfds);

        for (epoll_i = 0; epoll_i < epoll_nfds; epoll_i++) {
            sig_childblock();

            int evt_fd = epoll_evts[epoll_i].data.fd;
            int evt    = epoll_evts[epoll_i].events;
            log_debug("there is event fd:%d event:%d (%d/%d)", evt_fd, evt, epoll_i+1, epoll_nfds);

			int listen_fd = -1;
			int fdv = 4;		// ip version
			int si = which_is_listen(evt_fd);
			if (si != -1) {
				listen_fd = listen_socks[si];
				fdv = listen_sock_types[si];
				log_debug("there is IPv%d event fd:%d", fdv, listen_fd);
			}

            if (evt & EPOLLERR) {
                // ------ 监控到错误事件 ------
                log_error("epoll error:[%d]%s", errno, strerror(errno));
                close_fd(evt_fd);
                continue;

            } else if (evt & EPOLLIN) {
                // ------ 监控到可读事件 ------
                if (evt_fd == listen_fd) {
                    // ------ 处理新接入的socket ------

                    int accept_times = 0;
                    while (1) {
                        accept_times++;
						client_sin_size = sizeof(client_addr);
						connfd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_sin_size);
                        if (connfd == -1) {
                            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) 
                                || (errno == EINTR) || (errno == EPROTO)) {
                                // 资源暂时不可读，再来一遍
                                if (accept_times > 5) {
                                    log_error("accept times(%d) error(%d):%s, No More Accept.", 
                                            accept_times, errno, strerror(errno));
                                    break;
                                }
                                continue;
                            } else {
                                log_error("accept error(%d):%s", errno, strerror(errno));
                                break;
                            }
                        }

                        break;
                    }

                    // Create Unique ID
                    char scid[MAXLINE] = {0};
                    create_unique_id(scid, sizeof(scid));
                    log_debug("%s create sid succ", scid);

                    // Get a new index from client lists
                    int i = get_idle_idx_from_clients();
                    if (i == -1) {
                        log_error("%s get idle index from client list fail: maybe client queue is full.", scid);
                        safe_write(connfd, "system error\n", 13, atoi(crw_timeout));;
                        close(connfd);
                        goto next_epoll;
                    }

                    clients_t[i].used = 1;
                    clients_t[i].fd = connfd;
                    snprintf(clients_t[i].sid, sizeof(clients_t[i].sid), "%s", scid);

                    // Get client IP and Port
					char cip[INET6_ADDRSTRLEN];
					inet_ntop(client_addr.ss_family, 
							get_in_addr((struct sockaddr *)&client_addr),
							cip, sizeof(cip));
					int cport = ntohs(get_in_port((struct sockaddr *)&client_addr));

					snprintf(clients_t[i].ip, sizeof(clients_t[i].ip), "%s", cip);
					snprintf(clients_t[i].port, sizeof(clients_t[i].port), "%d", cport);

					conv_ip_to_hex_str_family(client_addr.ss_family, cip,
										clients_t[i].ip_hex, sizeof(clients_t[i].ip_hex));

                    log_info("got IPv%d connect from %s(%s) port %s", fdv, 
									clients_t[i].ip, clients_t[i].ip_hex, clients_t[i].port);

                    // Fork child and exec
                    int ret = create_child_and_exec_with_idx_clientfd(i, listen_fd, connfd);
                    if (ret != 0) {
                        char error_buf[] = "Server Temporarily Error, Please Try Again Later\n";
                        safe_write(connfd, error_buf, strlen(error_buf), atoi(crw_timeout));
                        close_fd(connfd);
                        goto next_epoll;
                    }

                    epoll_num_running++;
                    goto next_epoll;

                } else {
                    // ------ 处理子进程可读数据 ------
                    // Get child sid
                    char child_mid[MAXLINE] = {0};
                    int idx = get_idx_with_sockfd(evt_fd);
                    if (idx < 0 ) {
                        log_error("get child index with socket fd:%d fail", evt_fd);
                        snprintf(child_mid, sizeof(child_mid), "00000000");
                    } else {
                        log_debug("get_idx_with_sockfd(%d) idx[%d]", evt_fd, idx);
                        snprintf(child_mid, sizeof(child_mid), "%s", clients_t[idx].sid);
                    }

                    // while (1) ...
                    // ...
                }

            } else if ((evt & EPOLLHUP) && (evt_fd != listen_fd)) {
                // ------ 有子进程退出 ------
                int idx = get_idx_with_sockfd(evt_fd);
                if (idx < 0) {
                    log_error("get index with socket fd[%d] fail, maybe memory leak", evt_fd);
                    close_fd(evt_fd);
                } else {
                    log_debug("%s get event EPOLLHUP: epoll_i[%d] fd[%d] used[%d]",
                            clients_t[idx].sid, epoll_i, epoll_evts[epoll_i].data.fd, clients_t[idx].used);
                    
                    clean_client_with_idx(idx);
                    close_fd(evt_fd);
                }

                epoll_num_running--;
            }

next_epoll:
            sig_childunblock();
        }
    }

    close_fd(epoll_fd);

	for (epoll_i=0; epoll_i<num_listen_socks; epoll_i++) {
    	close_fd(listen_socks[epoll_i]);
	}

    if (clients_t != NULL) {
        free(clients_t);
        clients_t = NULL;
    }
    if (epoll_evts != NULL) {
        free(epoll_evts);
        epoll_evts = NULL;
    }

    log_info("/******** I am normal quit, bye bye... ********/");

    return 0;
}





