epoll 网络模型
========================

Intro
-----

使用epoll做网络模型，父进程accept连接，创建子进程处理，
子进程文件描述符（0，1，2）分别dup成（pfd_r, pfd_w, client_fd)

client_fd:	客户端fd
pfd_r:		父进程 读fd
pfd_w:		父进程 写fd


#### 文件描述符
-----
#### 父进程         子进程     客户端
#### pfd_r <------  0
#### pfd_w ------>  1
####                2  <------>  fd
####


依赖:
-----

libgs 库:
	wget http://blog.vmeila.com/software/libgs-0.24.tar.gz

	tar xzvf libgs-0.24.tar.gz
	cd libgs-0.24/
	./configure --prefix=/usr/local/libgs/
	make 
	make install

	echo "/usr/local/libgs/lib64/libgs/base64" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/confparser" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/ctlog" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/dictionary" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/hashmap" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/mfile" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/stralloc" >> /etc/ld.so.conf.d/libgs-x86_64.conf"
	echo "/usr/local/libgs/lib64/libgs/utf8str" >> /etc/ld.so.conf.d/libgs-x86_64.conf"	
	
	/sbin/ldconfig

uuid:

	yum -y install libuuid-devel


启动方式:

	1. 拷贝 service.sh 到 /etc/init.d/ 并改名为epoll_server
	2. 修改权限: chmod 755 /etc/init.d/epoll_server
	3. 启动/停止: service epoll_server {start|stop}g


