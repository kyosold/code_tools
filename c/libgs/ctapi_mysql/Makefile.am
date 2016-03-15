AUTOMAKE_OPTIONS=foreign

if BUILD_ctapi_mysql
# [动态库]
lib_LTLIBRARIES=libgs_ctapi_mysql.la
libgs_ctapi_mysql_la_SOURCES=ctapi_mysql.c ctapi_mysql.h
libgs_ctapi_mysql_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/base64/ 下
ctapimysqlincludedir=$(includedir)/ctapi_mysql
ctapimysqlinclude_HEADERS=ctapi_mysql.h

endif

libdir=$(prefix)/lib/ctapi_mysql
