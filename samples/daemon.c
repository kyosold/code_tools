#### 说明开始
# 该例子展示一个程序启动后脱离当前shell，并且生成
# /var/run/dt.pid 文件，用于保存当前该进程号，
# 同时防止启动多个实例。
#### 说明结束


#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <strings.h>
 
#define PID_FILE    "/var/run/dt.pid"
 
int daemon_init(char *chdir_path)
{
	// 1. 后台运行
	pid_t pid1 = fork();
	if (pid1 == -1) {
		printf("fork fail\n");
		exit(1);
	} else if (pid1 > 0) {
		// parent exit
        exit(0);
    } 

	// 2. 独立于控制终端
	if (setsid() == -1) {
		printf("setsid fail\n");
		exit(1);
	}

	// 3. 防止子进程(组长)获取控制终端
	pid_t pid2 = fork();
	if (pid2 == -1) {
		printf("fork fail\n");
		exit(1);
	} else if (pid2 > 0) {
		// parent exit
		exit(0);
	}

	// 4. 关闭打开的文件描述符
	int i;
	for (i=0; i<NOFILE; i++) {
		close(i);
	}

	// 5. 改变工作目录
	if (chdir_path != NULL) {
		chdir(chdir_path);
	}

	// 6. 清除文件创建掩码(umask)
    umask(0);  

	// 7. 处理信号
	signal(SIGCHLD, SIG_IGN);

    return(0);
}
 
void sig_term(int signo)
{
    if(signo == SIGTERM)
    {
        syslog(LOG_INFO, "program terminated."); 
        closelog(); 
        exit(0);
    }
}
 
// 保证只会有一个实例运行，并且pid写入到/var/run/dt.pid中
void alread_running()
{
    int fd = -1;
    char pid_buf[512] = {0};
    fd = open(PID_FILE, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        syslog(LOG_INFO, "fail to open file:%s", PID_FILE);
        exit(1);
    }
    struct flock lock;
    bzero(&lock, sizeof(lock));
    if ( fcntl(fd, F_GETLK, &lock) < 0 ) {
        syslog(LOG_ERR, "fail to fcntl F_GETLK");
        exit(1);
    }
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    if ( fcntl(fd, F_SETLK, &lock) < 0 ) {
        syslog(LOG_ERR, "fail to fcntl F_SETLK");
        exit(1);
    }
    pid_t pid = getpid();
    int len = snprintf(pid_buf, sizeof(pid_buf), "%lld\n", pid);
    write(fd, pid_buf, len);
}
 
 
int main(void)
{
	char chg_path[] = "/usr/local/kyosold/";
    if(daemon_init(chg_path) == -1) 
    {
        printf("can't fork self\n");
        exit(0);
    }
     
    openlog("daemontest", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "program started.");
    signal(SIGTERM, sig_term);  
     
    alread_running();
     
    while(1)
    {
        sleep(1);  
    }
    return(0);
}
