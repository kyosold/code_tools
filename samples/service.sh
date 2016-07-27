##### 使用方法
# 说明: 下面是启动 dt 服务的脚本，该 dt 服务会在启动后会生成
#      /var/run/dt.pid 的文件，该文件保存当前启动dt服务后
#      后在进程号，以前脚本会kill该进程。
# 使用: 
#      1. 拷贝下面的脚本到 /etc/init.d/dt
#      2. 修改权限: chmod 755 /etc/init.d/dt
#      3. $service dt start
#         $service dt stop
#         $service dt restart
##### 结束说明


############################## dt SCRIPT BEGIN
#!/bin/sh
# dt
#
# processname: /usr/local/bin/dt
# pidfile:     /var/run/dt.pid
 
[ -f /etc/rc.d/init.d/functions ] && . /etc/rc.d/init.d/functions
 
RETVAL=0    #使用变量作为判断和关联上下文的载体
 
start() {
    echo -n $"Starting dt: "
    /usr/local/bin/dt >/dev/null 2>&1
    RETVAL=$?
    echo
    [ $RETVAL -eq 0 ] && action "启动 dt:" /bin/true || action "启动 dt:" /bin/false
    return $RETVAL
}
 
stop() {
    echo -n $"Stopping dt:"
    kill `cat /var/run/dt.pid`
    RETVAL=$?
    echo
    [ $RETVAL -eq 0 ] && action "停止 dt:" /bin/true || action "启动 dt:" /bin/false
    return $RETVAL
}

case "$1" in
    start)
        start
        ;;  
    stop)
        stop
        ;;  
    restart)
        sh $0 stop
        sleep 2
        sh $0 start
        ;;
    *)
        echo "Format error!"
        echo $"Usage: $0 {start|stop|restart|"
        exit 1
        ;;
esac
exit $RETVAL
############################## SCRIPT END
