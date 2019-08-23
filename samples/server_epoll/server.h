#ifndef _SERVER_H
#define _SERVER_H

#define DEF_PORT        "3331"
#define DEF_MAX_WORKS   "1024"
#define DEF_LOG_LEVEL   "8"
#define DEF_RW_TIMEOUT	"2"
#define DEF_PID_FILE    "/var/run/rp_server.pid"


// ------
#define wait_crashed(w)     ((w) & 127)
#define wait_exitcode(w)    ((w) >> 8)


struct client_st
{
    int     used;       /* 是否正使用 */
    int     pid;        /* 子进程的pid */
    int     fd;         /* 客户端accept的fd */

    char    ip[20];     /* Client IP */
    char    port[20];   /* Client Port */
    char    sid[128];   /* Session ID */

    int     pfd_r;      /* 父进程从子进程读的fd */
    int     pfd_w;      /* 父进程向子进程写的fd */
};


#endif
