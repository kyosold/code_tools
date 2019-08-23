#include <stdio.h>
#include <string.h>
#include "ctlog.h"

int ctlog_level = info;
char ctlog_sid[1024] = {0};

void ctlog(const char *ident, int option, int facility)
{
    openlog(ident, option, facility);
}
