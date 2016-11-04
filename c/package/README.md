libgs 基础库
=======

Intro
-----

libgs为C语言的基础库，封装一些日常使用的基础库

说明
------------
#### 安装后配置动态库
```bash
# 1. 编译文件: # vim /etc/ld.so.conf.d/libgs-x86_64.conf
	/usr/local/libgs/lib64/libgs/base64
	/usr/local/libgs/lib64/libgs/confparser
	/usr/local/libgs/lib64/libgs/ctlog
	/usr/local/libgs/lib64/libgs/dictionary
	/usr/local/libgs/lib64/libgs/hashmap
	/usr/local/libgs/lib64/libgs/mfile
	/usr/local/libgs/lib64/libgs/stralloc
	/usr/local/libgs/lib64/libgs/utf8str
	/usr/local/libgs/lib64/libgs/ctapi_curl
	/usr/local/libgs/lib64/libgs/ctapi_mc

# 2. 执行: # /sbin/ldconfig
```

