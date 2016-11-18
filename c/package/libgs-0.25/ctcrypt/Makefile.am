AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_ctcrypt.la
libgs_ctcrypt_la_SOURCES=ctcrypt.c
libgs_ctcrypt_la_CFLAGS=-g -I./


# 安装该包到 ($prefix}/include/libgs/ctcrypt/ 下
ctcryptincludedir=$(includedir)/@PACKAGENAME@/ctcrypt
ctcryptinclude_HEADERS=ctcrypt.h

# 安装库文件到 {$prefix}/lib64/libgs/ctcrypt/ 下
libdir=$(prefix)/lib64/@PACKAGENAME@/ctcrypt

EXTRA_DIST=sample.c

ctcryptdatadir=$(datarootdir)/@PACKAGENAME@/ctcrypt
ctcryptdata_DATA=sample.c
