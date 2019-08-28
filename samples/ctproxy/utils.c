#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <strings.h>
#include <uuid/uuid.h>
#include <stdint.h>

#include "util.h"
#include "ctlog.h"



/**
 *  使用UUID，生成唯一ID
 *
 *  @param mid      返回的id buffer
 *  @param mid_size id buffer 大小
 *
 *  @return 16:成功  其它:失败
 */
int create_unique_id(char *mid, size_t mid_size)
{
    uuid_t uid;
    uuid_generate(uuid);
    unsigned char *p = uuid;

    int i;
    char ch[5] = {0};
    for (i = 0; i < sizeof(uuid_t); i++)
    {
        snprintf(ch, sizeof(ch), "%02X", *p);
        mid[i*2] = ch[0];
        mid[i*2+1] = ch[1];
    }

    return i;
}



/**
 * 设置为非阻塞
 *
 * @param fd 指定的描述符
 *
 * @return -1:失败 其它:成功
 */
int ndelay_on(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}


/**
 * 设置为阻塞
 *
 * @param fd 指定的描述符
 *
 * @return -1:失败 其它:成功
 */
int ndelay_off(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}


/**
 * fd 拷贝
 *
 * @param to    新的文件描述符
 * @param from  旧的文件描述符
 *
 * @return 0:成功 -1:失败
 */
int fd_copy(int to, int from)
{
    if (to == from)
        return 0;
    if (fcntl(from, F_GETFL, 0) == -1)
        return -1;
    close(to);
    if (fcntl(from, F_DUPFD, to) == -1)
        return -1;
    return 0;
}

/**
 * fd从from转移到to, from被关闭
 *
 * @param to
 * @param from
 *
 * @return 0:成功 -1:失败
 */
int fd_move(int to, int from)
{
    if (to == from)
        return 0;
    if (fd_copy(to, from) == -1)
        return -1;
    close(from);
    return 0;
}


/*******************************
 * Sig
 ******************************/
void sig_catch( int sig, void (*f) () )
{   
    struct sigaction sa;
    sa.sa_handler = f;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, (struct sigaction *) 0);
}
    
void sig_block(int sig)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, sig);
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *) 0);
}

void sig_unblock(int sig)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, sig);
    sigprocmask(SIG_UNBLOCK, &ss, (sigset_t *) 0);
}   
        
void sig_pipeignore()
{   
    sig_catch(SIGPIPE, SIG_IGN);
}

void sig_childblock()
{
    sig_block(SIGCHLD);
}

void sig_childunblock()
{
    sig_unblock(SIGCHLD);
}
