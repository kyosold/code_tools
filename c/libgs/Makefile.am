AUTOMAKE_OPTIONS=foreign

# 是否编译 ctapi_mc
if BUILD_ctapi_mc
USE_MC=ctapi_mc
endif

# 编译 ctapi_mysql
if BUILD_ctapi_mysql
USE_MYSQL=ctapi_mysql
endif

# 编译 ctapi_curl
if BUILD_ctapi_curl
USE_CURL=ctapi_curl
endif

# 编译 ctcrypt
if BUILD_ctcrypt
USE_OPENSSL=ctcrypt
endif


SUBDIRS=base64 confparser $(USE_MC) dictionary $(USE_MYSQL) hashmap libmfile libstralloc utf8str $(USE_CURL) ctlog $(USE_OPENSSL) cthash
