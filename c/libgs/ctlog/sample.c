#include <stdio.h>

#include "ctlog.h"

int main(int argc, char **argv)
{
	// 1. open maillog
	ctlog("log_name", LOG_PID|LOG_NDELAY, LOG_MAIL);

	// 2. set session unique id
	snprintf(ctlog_sid, sizeof(ctlog_sid), "tB24U1FL025441");

	// 3. set log level
	ctlog_level = info;

	// 4. write some log
	log_info("info log:%s", "hello, world");    # info
	log_debug("debug log:%s", "hello, world");  # debug
	log_error("error log:%s", "hello, world");  # error

	return 0;
}
