AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_base64.la
libgs_base64_la_SOURCES=base64.c
libgs_base64_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/base64/ 下
base64includedir=$(includedir)/base64
base64include_HEADERS=base64.h

libdir=$(prefix)/lib/base64
