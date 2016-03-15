AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_confparser.la
libgs_confparser_la_SOURCES=iniparser.c dictionary.c confparser.c
libgs_confparser_la_CFLAGS=-g -I./

# 拷贝头文件到{$prefix}/include/confparser/下
confparserincludedir=$(includedir)/confparser
confparserinclude_HEADERS=iniparser.h dictionary.h confparser.h

# 拷贝库文件到{$prefix}/lib/confparser/下
libdir=$(prefix)/lib/confparser
