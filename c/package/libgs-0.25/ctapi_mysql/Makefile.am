AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_ctapi_mysql.la
libgs_ctapi_mysql_la_SOURCES=ctapi_mysql.c ctapi_mysql.h
libgs_ctapi_mysql_la_CFLAGS=-g -I./ -I/usr/include/

# 安装头文件到 {$prefix}/include/libgs/ctapi_mysql/ 下
ctapimysqlincludedir=$(includedir)/@PACKAGENAME@/ctapi_mysql
ctapimysqlinclude_HEADERS=ctapi_mysql.h

libdir=$(prefix)/lib64/@PACKAGENAME@/ctapi_mysql

EXTRA_DIST=sample.c

ctapimysqldatadir=$(datarootdir)/@PACKAGENAME@/ctapi_mysql
ctapimysqldata_DATA=sample.c
