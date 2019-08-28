#ifndef __CTLOG_H_
#define __CTLOG_H_

#include "syslog.h"
#include "utils.h"

extern int ctlog_level;
extern char ctlog_sid[MAXLINE];


#define CTLOG_DEBUG     8
#define CTLOG_INFO      7
#define CTLOG_NOTICE    6
#define CTLOG_WARNING   5
#define CTLOG_ERROR     4
#define CTLOG_CRIT      3
#define CTLOG_ALERT     2
#define CTLOG_EMERG     1


#define log_emerg(fmt, ...) { \
    if (ctlog_level >= CTLOG_EMERG) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_EMERG, "[EMERG] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_EMERG, "[EMERG] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_alert(fmt, ...) { \
    if (ctlog_level >= CTLOG_ALERT) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_ALERT, "[ALERT] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_ALERT, "[ALERT] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_crit(fmt, ...) { \
    if (ctlog_level >= CTLOG_CRIT) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_CRIT, "[CRIT] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_CRIT, "[CRIT] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_error(fmt, ...) { \
    if (ctlog_level >= CTLOG_ERROR) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_ERR, "[ERROR] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_ERR, "[ERROR] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_warning(fmt, ...) { \
    if (ctlog_level >= CTLOG_WARNING) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_WARNING, "[WARNING] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_WARNING, "[WARNING] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_notice(fmt, ...) { \
    if (ctlog_level >= CTLOG_NOTICE) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_NOTICE, "[NOTICE] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_NOTICE, "[NOTICE] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_info(fmt, ...) { \
    if (ctlog_level >= CTLOG_INFO) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_INFO, "[INFO] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_INFO, "[INFO] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}

#define log_debug(fmt, ...) { \
    if (ctlog_level >= CTLOG_DEBUG) { \
        if (*ctlog_sid == '\0') { \
            syslog(LOG_DEBUG, "[DEBUG] %s "fmt, __func__, ##__VA_ARGS__); \
        } else { \
            syslog(LOG_DEBUG, "[DEBUG] %s %s "fmt, __func__, ctlog_sid, ##__VA_ARGS__); \
        } \
    } \
}


void ctlog_open(const char *ident, int opt, int facility);

#endif
