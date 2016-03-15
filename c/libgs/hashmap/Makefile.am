AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_hashmap.la
libgs_hashmap_la_SOURCES=hashmap.c
libgs_hashmap_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/hashmap/ 下
hashmapincludedir=$(includedir)/hashmap
hashmapinclude_HEADERS=hashmap.h

libdir=$(prefix)/lib/hashmap
