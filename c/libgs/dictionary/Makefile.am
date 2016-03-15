AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libgs_dictionary.la
libgs_dictionary_la_SOURCES=dictionary.c
libgs_dictionary_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/dictionary/ 下
dictionaryincludedir=$(includedir)/dictionary
dictionaryinclude_HEADERS=dictionary.h

libdir=$(prefix)/lib/dictionary
