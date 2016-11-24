/*
 * 编译: gcc -g -o sample sample.c -I/usr/local/libgs/include/libgs/cthash/ -L/usr/local/libgs/lib64/libgs/cthash/ -lgs_cthash
 *
 * 如果想crc32一个文件，使用下面的crc32_file函数
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "cthash.h"


static int crc32_file(const char *in_file, unsigned long *crc32_val);

int main(int argc, char **argv)
{
    char *str = argv[1];

    printf("simple Hash:%d\n", simple_hash(str));
    printf("SDBM Hash:%d\n", SDBM_hash(str));
    printf("RS Hash:%d\n", RS_hash(str));
    printf("JS Hash:%d\n", JS_hash(str));
    printf("PJW Hash:%d\n", PJW_hash(str));
    printf("ELF Hash:%d\n", ELF_hash(str));
    printf("BKDR Hash:%d\n", BKDR_hash(str));
    printf("DJB Hash:%d\n", DJB_hash(str));
    printf("AP Hash:%d\n", AP_hash(str));
    printf("CRC Hash:%d\n", CRC_hash(str));

    // crc32 字符串
    printf("CRC32 Hash:%ld\n", CRC32_hash(0, str, strlen(str)));

    // crc32 文件
    int ret = access(str, F_OK);
    if (ret == 0) {
        unsigned long crc32_val;
        ret = crc32_file(str, &crc32_val);
        if (ret < 0) {
            return 1;
        }
        printf("CRC32 Hash File:%ld\n", crc32_val);
        printf("CRC32 Hash File:%08x\n", crc32_val);
    } else {
        printf("ret[%d] error:%s\n", ret, strerror(errno));
    }


    return 0;
}


/*
 *计算大文件的CRC校验码:crc32函数,是对一个buffer进行处理,
 *但如果一个文件相对较大,显然不能直接读取到内存当中
 *所以只能将文件分段读取出来进行crc校验,
 *然后循环将上一次的crc校验码再传递给新的buffer校验函数,
 *到最后，生成的crc校验码就是该文件的crc校验码.(经过测试)
*/
static int crc32_file(const char *in_file, unsigned long *crc32_val)
{
    int fd, nr, ret;
    unsigned char buf[1024];

    FILE *fp = fopen(in_file, "rb");
    if (fp == NULL) {
        printf("%s fopen fail:%s\n", __LINE__, strerror(errno));
        return -1;
    }

    // 第一次传入的值需要固定,如果发送端使用该值计算crc校验码,
    // 那么接收端也同样需要使用该值进行计算
    unsigned long crc = 0;

    while (1) {
        nr = fread(buf, 1, sizeof(buf), fp);
        if (nr == 0) {
            if (ferror(fp)) {
                printf("%s read file fail:%s\n", __LINE__, strerror(errno));
                return -1;
            }
            break;
        }
        //printf("nr[%d] crc[%ld]\n", nr, crc);
        crc = CRC32_hash(crc, buf, nr);
    }

    fclose(fp);

    *crc32_val = crc;

    return 0;

}

