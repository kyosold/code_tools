AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_utf8str.la
libgs_utf8str_la_SOURCES=utf8str.c
libgs_utf8str_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/libgs/utf8str/ 下
utf8strincludedir=$(includedir)/@PACKAGENAME@/utf8str
utf8strinclude_HEADERS=utf8str.h

libdir=$(prefix)/lib64/@PACKAGENAME@/utf8str
