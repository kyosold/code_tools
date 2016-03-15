AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_mfile.la
libgs_mfile_la_SOURCES=mfile.c
libgs_mfile_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/mfile/ 下
mfileincludedir=$(includedir)/mfile
mfileinclude_HEADERS=mfile.h

libdir=$(prefix)/lib/mfile
