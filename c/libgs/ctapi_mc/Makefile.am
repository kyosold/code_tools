AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_ctapi_mc.la
libgs_ctapi_mc_la_SOURCES=ctapi_mc.c ctlog.c
libgs_ctapi_mc_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/libgs/ctapi_mc/ 下
ctapimcincludedir=$(includedir)/@PACKAGENAME@/ctapi_mc
ctapimcinclude_HEADERS=ctapi_mc.h ctlog.h

libdir=$(prefix)/lib64/@PACKAGENAME@/ctapi_mc

EXTRA_DIST=sample.c linux.mk
