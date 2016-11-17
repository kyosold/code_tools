/*
 * 使用方法:
 *  # gcc -g -o sample sample.c -I/usr/local/libgs/include/libgs/ctcrypt -L/usr/local/libgs/lib64/libgs/ctcrypt/ -lgs_ctcrypt
 *  # ./sample 'hello, world'
 *
 * 注意：
 * 1. libgs 安装目录在/usr/local/libgs
 * 2. 安装时要添加上ctcrypt库:
 *      ./configure --prefix=/usr/local/libgs --with-ctcrypt
 * 3. 此库需要依赖openssl, 所以确保安装了openssl-devel库
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ctcrypt.h"


int main(int argc, char **argv)
{

    char out_string[1024] = {0};
    int ret = 0;

    ret = ct_md5(argv[1], strlen(argv[1]), out_string, sizeof(out_string));
    printf("md5     ret[%d]:%s\n", ret, out_string);

    memset(out_string, 0, sizeof(out_string));
    ret = ct_sha1(argv[1], strlen(argv[1]), out_string, sizeof(out_string));
    printf("sha1    ret[%d]:%s\n", ret, out_string);
    
    memset(out_string, 0, sizeof(out_string));
    char key[] = "012345678";
    ret = ct_hash_hmac("sha1", argv[1], strlen(argv[1]), key, strlen(key), out_string, sizeof(out_string));
    printf("hmac    ret[%d]:%s\n", ret, out_string);

    return 0;

}

