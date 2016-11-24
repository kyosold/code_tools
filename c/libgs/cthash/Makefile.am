AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_cthash.la
libgs_cthash_la_SOURCES=cthash.c
libgs_cthash_la_CFLAGS=-g -I./

# 安装该包到 ($prefix}/include/libgs/cthash/ 下
cthashincludedir=$(includedir)/@PACKAGENAME@/cthash
cthashinclude_HEADERS=cthash.h


# 安装库文件到 {$prefix}/lib64/libgs/cthash/ 下
libdir=$(prefix)/lib64/@PACKAGENAME@/cthash

EXTRA_DIST=sample.c

cthashdatadir=$(datarootdir)/@PACKAGENAME@/cthash
cthashdata_DATA=sample.c
