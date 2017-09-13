# cp service.sh to /etc/init.d/epoll_server
# chmod 755 /etc/init.d/epoll_server
# service epoll_server {start|stop}
#
#
#!/bin/bash
#
# process name: epoll_server
# pid file: /var/run/epoll_server.pid

[ -f /etc/rc.d/init.d/functions ] && . /etc/rc.d/init.d/functions

PIDFILE=/var/run/epoll_server.pid

HOMEDIR=/usr/local/server_epoll/

RETVAL=0    #使用变量作为判断和关联上下文的载体

start() {
    echo -n $"Starting ldap_proxy serivce ...: "
    if [ ! -f $PIDFILE ]; then
        $HOMEDIR/bin/epoll_server -c $HOMEDIR/etc/config.ini & 2>&1
        RETVAL=$?
        echo
        if [ $RETVAL -eq 0 ]; then
            action "Start epoll_server service:" /bin/true 
        
            pid=$(ps -ef | grep epoll_server | grep -v grep | awk '{print $2}')
            echo $pid > $PIDFILE
        else
            action "Start epoll_server service:" /bin/false 
        fi

        return $RETVAL

    else
        echo
        action "Start epoll_server service:" /bin/false

        return 1
    fi
        
}

stop() {
    echo -n $"Stopping epoll_server serivce ...: "
    if [ -f $PIDFILE ]; then
        kill `cat $PIDFILE`
        RETVAL=$?
        echo
        if [ $RETVAL -eq 0 ]; then 
            action "Stop epoll_server service:" /bin/true
    
            rm -f $PIDFILE
        else
            action "Stop epoll_server service:" /bin/false
        fi

        return $RETVAL 
    else
        echo
        action "Stop epoll_server service:" /bin/false

        return 1
    fi
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
        echo "Format error"
        echo $"Usage: $0 {start|stop|"
        exit 1
        ;;
esac
exit $RETVAL