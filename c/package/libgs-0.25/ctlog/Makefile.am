AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_ctlog.la
libgs_ctlog_la_SOURCES=ctlog.c
libgs_ctlog_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/ctlog/ 下
#ctlogincludedir=$(includedir)/ctlog
#ctloginclude_HEADERS=ctlog.h

# 安装库文件到 {$prefix}/lib/ctlog/ 下
#libdir=$(prefix)/lib/ctlog


# 安装该包到 ($prefix}/include/libgs/ctlog/ 下
ctlogincludedir=$(includedir)/@PACKAGENAME@/ctlog
ctloginclude_HEADERS=ctlog.h

# 安装库文件到 {$prefix}/lib64/libgs/ctlog/ 下
libdir=$(prefix)/lib64/@PACKAGENAME@/ctlog

EXTRA_DIST=sample.c

ctlogdatadir=$(datarootdir)/@PACKAGENAME@/ctlog
ctlogdata_DATA=sample.c
