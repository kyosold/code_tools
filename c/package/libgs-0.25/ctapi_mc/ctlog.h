#ifndef _LIAO_LOG_H
#define _LIAO_LOG_H

#include "syslog.h"

extern int log_level;

#define debug   8
#define info    7   
#define notice	6
#define warning	5
#define	error	4
#define	crit	3
#define	alert	2
#define	emerg	1


#define log_emerg(fmt, ...) {if(log_level>=emerg){syslog(LOG_EMERG, "[EMERG] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_alert(fmt, ...) {if(log_level>=alert){syslog(LOG_ALERT, "[ALERT] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_crit(fmt, ...) {if(log_level>=crit){syslog(LOG_CRIT, "[CRIT] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_error(fmt, ...) {if(log_level>=error){syslog(LOG_ERR, "[ERROR] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_warning(fmt, ...) {if(log_level>=warning){syslog(LOG_WARNING, "[WARNING] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_notice(fmt, ...) {if(log_level>=notice){syslog(LOG_NOTICE, "[NOTICE] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_info(fmt, ...) {if(log_level>=info){syslog(LOG_INFO, "[INFO] %s "fmt, __func__, ##__VA_ARGS__);}}
#define log_debug(fmt, ...) {if(log_level>=debug){syslog(LOG_DEBUG, "[DEBUG] %s "fmt, __func__, ##__VA_ARGS__);}}

void ctlog(const char *ident, int option, int facility);

#endif
