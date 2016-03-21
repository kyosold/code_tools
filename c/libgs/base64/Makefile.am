AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_base64.la
libgs_base64_la_SOURCES=base64.c
libgs_base64_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/base64/ 下
#base64includedir=$(includedir)/base64
#base64include_HEADERS=base64.h

# 安装库文件到 {$prefix}/lib/base64/ 下
#libdir=$(prefix)/lib/base64


# 安装该包到 ($prefix}/include/libgs/base64/ 下
base64includedir=$(includedir)/@PACKAGENAME@/base64
base64include_HEADERS=base64.h

# 安装库文件到 {$prefix}/lib64/libgs/base64/ 下
libdir=$(prefix)/lib64/@PACKAGENAME@/base64

EXTRA_DIST=sample.c
