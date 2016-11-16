#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libgs], [0.24], [kyosold@qq.com])
AC_CONFIG_SRCDIR([base64/base64.c])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for programs.
AC_PROG_CC


# ======================================
# --with-ctapi_mc ----------
AC_ARG_WITH(
	[ctapi_mc],
	[AS_HELP_STRING([--with-ctapi_mc], [Enable ctapi_mc Library])],
	[zmq_ctapi_mc="yes"],
	[zmq_ctapi_mc="no"]
)
# Checks for libraries.
if test "x$zmq_ctapi_mc" == "xyes"; then
	AC_CHECK_LIB([memcached], [memcached_set], [], [AC_MSG_ERROR([Could not find library -lmemcached])])
fi

# ======================================
# --with-ctapi_mysql -------
AC_ARG_WITH(
	[ctapi_mysql],
	[AS_HELP_STRING([--with-ctapi_mysql], [Enable ctapi_mysql Library])],
	[zmq_ctapi_mysql="yes"],
	[zmq_ctapi_mysql="no"]
)
# Check mysql libraries
if test "x$zmq_ctapi_mysql" == "xyes"; then
	AC_CHECK_LIB([mysqlclient], [mysql_real_query], [], [AC_MSG_ERROR([Could not find library -lmysqlclient])])
fi

# ======================================
# --with-ctapi_curl --------
AC_ARG_WITH(
	[ctapi_curl],
	[AS_HELP_STRING([--with-ctapi_curl], [Enable ctapi_curl Library])],
	[zmq_ctapi_curl="yes"],
	[zmq_ctapi_curl="no"]
)
# Check curl libraries
if test "x$zmq_ctapi_curl" == "xyes"; then
	AC_CHECK_LIB([curl], [curl_easy_init], [], [AC_MSG_ERROR([Could not find library -lcurl])])
fi


AM_CONDITIONAL(BUILD_ctapi_mc, test "x$zmq_ctapi_mc" == "xyes")
AM_CONDITIONAL(BUILD_ctapi_mysql, test "x$zmq_ctapi_mysql" == "xyes")
AM_CONDITIONAL(BUILD_ctapi_curl, test "x$zmq_ctapi_curl" == "xyes")

AC_SUBST([PACKAGENAME], ["libgs"])

AC_PROG_LIBTOOL


# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h netdb.h stddef.h stdint.h stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memchr memmove memset strcasecmp strchr strdup strncasecmp strtol])

AC_OUTPUT([Makefile 
		base64/Makefile 
		confparser/Makefile 
		dictionary/Makefile 
		ctapi_mc/Makefile 
		ctapi_mysql/Makefile 
		hashmap/Makefile 
		libmfile/Makefile 
		libstralloc/Makefile 
		utf8str/Makefile 
		ctapi_curl/Makefile 
		ctlog/Makefile])
