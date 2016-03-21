AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_ctapi_curl.la
libgs_ctapi_curl_la_SOURCES=ctapi_curl.c ctapi_curl.h
libgs_ctapi_curl_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/libgs/ctapi_curl/ 下
ctapimcincludedir=$(includedir)/@PACKAGENAME@/ctapi_curl
ctapimcinclude_HEADERS=ctapi_curl.h

# 安装证书文件到 ($prefix}/share/libgs/ctapi_curl/ 下
ctapimcdatadir=$(datarootdir)/@PACKAGENAME@/ctapi_curl
ctapimcdata_DATA=cacert.pem

EXTRA_DIST=$(ctapimcdata_DATA) sample.c

libdir=$(prefix)/lib64/@PACKAGENAME@/ctapi_curl
