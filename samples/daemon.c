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
 
int daemon_init(void)
{
    pid_t pid;
    if((pid = fork()) < 0)
    {
        return(-1);
    }
    else if(pid != 0)
    {
        exit(0);
    } 
    setsid();  
    chdir("/");  
    umask(0);  
    close(0);
    close(1);  
    close(2);  
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
    if(daemon_init() == -1) 
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
