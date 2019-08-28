#include <stdio.h>
#include <string.h>

#include "ctlog.h"

int ctlog_level         = CTLOG_INFO;
char ctlog_sid[MAXLINE] = {0};


void ctlog_open(const char *ident, int opt, int facility)
{
    openlog(ident, opt, facility);
}
