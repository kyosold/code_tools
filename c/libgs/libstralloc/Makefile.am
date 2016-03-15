AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_stralloc.la
libgs_stralloc_la_SOURCES=stralloc.c
libgs_stralloc_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/stralloc/ 下
strallocincludedir=$(includedir)/stralloc
strallocinclude_HEADERS=stralloc.h

libdir=$(prefix)/lib/stralloc
