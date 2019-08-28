#ifndef __UTILS_H_
#define __UTILS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINE     1024


#define wait_crashed(w)     ((w) & 127)
#define wait_exitcode(w)    ((w) >> 8)
#define wait_stopsig(w)     ((w) >> 8)
#define wait_stopped(w)     (((w) & 127) == 127)


/*********************************
 * functions
 ********************************/

/**
 *  使用UUID，生成唯一ID
 *
 *  @param mid      返回的id buffer
 *  @param mid_size id buffer 大小
 *
 *  @return 16:成功  其它:失败
 */
int create_unique_id(char *mid, size_t mid_size);


/**
 * 设置为非阻塞
 *
 * @param fd 指定的描述符
 *
 * @return -1:失败 其它:成功
 */
int ndelay_on(int fd);


/**
 * 设置为阻塞
 *
 * @param fd 指定的描述符
 *
 * @return -1:失败 其它:成功
 */
int ndelay_off(int fd);



/**
 * fd 拷贝
 *
 * @param to    新的文件描述符
 * @param from  旧的文件描述符
 *
 * @return 0:成功 -1:失败
 */
int fd_copy(int to, int from);

/**
 * fd从from转移到to, from被关闭
 *
 * @param to
 * @param from
 *
 * @return 0:成功 -1:失败
 */
int fd_move(int to, int from);



void sig_catch(int sig, void (*f) ());
void sig_block(int sig);
void sig_unblock(int sig);
void sig_pipeignore();
void sig_childblock();
void sig_childunblock();




#endif