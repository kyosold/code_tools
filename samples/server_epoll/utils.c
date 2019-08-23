#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/epoll.h>
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
#include <strings.h>
#include <uuid/uuid.h>

#include "utils.h"

#include "ctlog.h"


// --------------------------------------------
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
 * 从fd中读数据到buf
 *
 * @param fd    指定的描述符
 * @param buf   存储数据的buf
 * @param len   buf的内容空间size
 * @param timeout   读数据超时时间,单位:秒
 *
 * @return -1:系统错误 -2:读超时 >=0:读到的数据长度
 */
int read_fd_timeout(int fd, char *buf, int len, int timeout)
{
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    if (select(fd + 1, &rfds, (fd_set *)0, (fd_set *)0, &tv) == -1) {
        log_error("select fail:[%d]%s", errno, strerror(errno));
        return -1;
    }
    if (FD_ISSET(fd, &rfds)) {
        return read(fd, buf, len);
    }
    log_error("read data from remote host timeout");

    return -2;
}


/**
 * 写buf内数据到fd
 *
 * @param fd    指定的描述符
 * @param buf   存放数据的buf
 * @param len   buf的内容空间size
 * @param timeout   写数据时超时时间,单位:秒
 *
 * @return -1:系统错误 -2:写超时 >=0:写入的数据长度
 */
int write_fd_timeout(int fd, char *buf, int len, int timeout)
{
    if (ndelay_on(fd) == -1) {
        log_error("set nonblock fail:[%d]%s", errno, strerror(errno));
        return -1;
    }

    fd_set  wfds;
    struct timeval tv;

    int wrote_len   = 0;
    char *pbuf      = buf;
    int nw          = 0;
    int n           = len;

    while (n > 0) {
        tv.tv_sec   = timeout;
        tv.tv_usec  = 0;

        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);

        int ret = select(fd + 1, (fd_set *)0, &wfds, (fd_set *)0, &tv);
        if (ret == -1) {
            log_error("select fail:[%d]%s", errno, strerror(errno));
            ndelay_off(fd);

            return -1;
        } else if (ret == 0) {
            // timeout
            log_error("write data to remote host timeout");
            ndelay_off(fd);

            return -2;
        } else {
            if (FD_ISSET(fd, &wfds)) {
                nw = write(fd, pbuf + wrote_len, n);
                log_debug("write len:%d/%d [%s]", nw, n, pbuf+wrote_len);

                if ( (nw == -1) && ((errno != EAGAIN) || (errno != EINTR)) ) {
                    log_error("write to remote host fail:[%d]%s", errno, strerror(errno));
                    ndelay_off(fd);
                    return -1;
                } else if (nw == 0) {
                    log_error("remote host closed, but i don't send data [%d] to finished\n", n);
                    return -1;
                } else {
                    wrote_len += nw;
                    n -= nw;
                }
            }
        }
    }

    ndelay_off(fd);

    return wrote_len;
}


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
    uuid_t uuid;
    uuid_generate(uuid);

    unsigned char *p = uuid;
    int i;
    char ch[5] = {0};
    for (i=0; i<sizeof(uuid_t); i++,p++) {
        snprintf(ch, sizeof(ch), "%02X", *p);
        mid[i*2] = ch[0];
        mid[i*2+1] = ch[1];
    }

    return i;
}


// -------- FD 操作 ---------
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

// -------- 信号处理 --------
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

