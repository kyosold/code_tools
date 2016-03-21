AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_mfile.la
libgs_mfile_la_SOURCES=mfile.c
libgs_mfile_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/libgs/mfile/ 下
mfileincludedir=$(includedir)/@PACKAGENAME@/mfile
mfileinclude_HEADERS=mfile.h queue.h

libdir=$(prefix)/lib64/@PACKAGENAME@/mfile

EXTRA_DIST=sample.c linux.mk
