#ifndef _UTILS_H
#define _UTILS_H

#define CHILD_PROG      "./child"

#define MAX_LINE        1024
#define NET_FAIL		0


#define wait_crashed(w)     ((w) & 127)
#define wait_exitcode(w)    ((w) >> 8)
#define wait_stopsig(w)     ((w) >> 8)
#define wait_stopped(w)     (((w) & 127) == 127)




/**
 *  使用UUID，生成唯一ID
 *
 *  @param mid      返回的id buffer
 *  @param mid_size id buffer 大小
 *
 *  @return 16:成功  其它:失败
 */
int create_unique_id(char *mid, size_t mid_size);


/////////////// FD 阻塞/非阻塞 操作 /////////////////////

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


/////////////// FD 读写 操作 /////////////////////

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
int read_fd_timeout(int fd, char *buf, int len, int timeout);


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
int write_fd_timeout(int fd, char *buf, int len, int timeout);


/////////////// FD 操作 /////////////////////

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


/////////////// 信号 操作 /////////////////////
void sig_catch( int sig, void (*f) () );
void sig_block(int sig);
void sig_unblock(int sig);
void sig_pipeignore();
void sig_childblock();
void sig_childunblock();


#endif
