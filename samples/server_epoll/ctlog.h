#ifndef _LIAO_LOG_H
#define _LIAO_LOG_H

#include "syslog.h"

extern int ctlog_level;
extern char ctlog_sid[1024];

#define debug   8
#define info    7   
#define notice  6
#define warning 5
#define error   4
#define crit    3
#define alert   2
#define emerg   1


#define log_emerg(fmt, ...) {   \
    if(ctlog_level>=emerg){     \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_EMERG, "[EMERG] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_EMERG, "[EMERG] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_alert(fmt, ...) { \
    if(ctlog_level>=alert){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_ALERT, "[ALERT] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_ALERT, "[ALERT] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_crit(fmt, ...) { \
    if(ctlog_level>=crit){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_CRIT, "[CRIT] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_CRIT, "[CRIT] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_error(fmt, ...) { \
    if(ctlog_level>=error){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_ERR, "[ERROR] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_ERR, "[ERROR] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_warning(fmt, ...) { \
    if(ctlog_level>=warning){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_WARNING, "[WARNING] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_WARNING, "[WARNING] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_notice(fmt, ...) { \
    if(ctlog_level>=notice){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_NOTICE, "[NOTICE] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_NOTICE, "[NOTICE] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_info(fmt, ...) { \
    if(ctlog_level>=info){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_INFO, "[INFO] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_INFO, "[INFO] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_debug(fmt, ...) { \
    if(ctlog_level>=debug){ \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_DEBUG, "[DEBUG] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_DEBUG, "[DEBUG] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

void ctlog(const char *ident, int option, int facility);

#endif
