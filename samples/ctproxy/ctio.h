#ifndef __CTIO_H_
#define __CTIO_H_

#include <sys/types.h>

size_t safe_read(int fd, char *buf, size_t len, int timeout_seconds);

size_t safe_write(int fd, const char *buf, size_t len, int timeout_seconds);

int read_line(int fd, char *buf, size_t bufsiz, int rtimeout);

#endif