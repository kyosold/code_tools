AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_ctapi_curl.la
libgs_ctapi_curl_la_SOURCES=ctapi_curl.c ctapi_curl.h
libgs_ctapi_curl_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/libgs/ctapi_curl/ 下
ctapicurlincludedir=$(includedir)/@PACKAGENAME@/ctapi_curl
ctapicurlinclude_HEADERS=ctapi_curl.h

libdir=$(prefix)/lib64/@PACKAGENAME@/ctapi_curl

EXTRA_DIST=$(ctapimcdata_DATA) sample.c

# 安装证书文件到 ($prefix}/share/libgs/ctapi_curl/ 下
ctapicurldatadir=$(datarootdir)/@PACKAGENAME@/ctapi_curl
ctapicurldata_DATA=cacert.pem sample.c
