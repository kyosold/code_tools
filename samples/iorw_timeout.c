#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>


#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif


int ndelay_on(int fd)
{
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int ndelay_off(int fd)
{
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}


// reutrn:
// -1: system fail
// -2: timeout
// >0: written length
int write_nb_timeout(int fd, char *buf, int len, int timeout)
{
	if (ndelay_on(fd) == -1) {
		printf("set nonblock fail:%s\n", strerror(errno));
		return -1;
	}

	int wrote_len = 0;
	char *pbuf = buf;
	int nwrite = 0;
	int n = len;

	fd_set wfds;
	struct timeval tv;

	while (n > 0) {
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		FD_ZERO(&wfds);
		FD_SET(fd, &wfds);

		int ret = select(fd + 1, (fd_set *) 0, &wfds, (fd_set *) 0, &tv);
		if ( ret == -1 ) {
			//  select fail
			printf("select fail:%s\n", strerror(errno));
			ndelay_off(fd);
			return -1;

		} else if ( ret == 0 ) {
			//  timeout
			printf("write data to remote host timeout[%d]:%s\n", timeout, strerror(errno));
			ndelay_off(fd);
			return -2;

		} else {
			if (FD_ISSET(fd, &wfds)) {
				nwrite = write(fd, pbuf + wrote_len, n);
				printf("write len:%d/%d [%s]\n", nwrite, n, pbuf + wrote_len);

				if ( (nwrite == -1) && ((errno != EAGAIN) || (errno != EINTR)) ) {
					printf("write to remote host fail:[%d]%s\n", errno, strerror(errno));
					ndelay_off(fd);
					return -1;

				} else if ( nwrite == 0 ) {
					printf("remote host closed, but i don't send data [%d] to finished\n", n);
					ndelay_off(fd);
					return -1;

				} else {
					wrote_len += nwrite;
					n -= nwrite;

				}
			}

		}
	}

	ndelay_off(fd);
	return wrote_len;
}




int read_nb_timeout(int fd, char *buf, int len, int timeout)
{
	fd_set rfds;
	struct timeval tv;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	if (select(fd + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv) == -1) {
		//  select fail
		printf("select fail:%s\n", strerror(errno));
		return -1;
	}

	if (FD_ISSET(fd, &rfds)) {
		return read(fd, buf, len);
	}

	printf("read data from remote host timeout[%d]\n", timeout);
	return -2;
}


int main(int argc, char **argv)
{
	char buf[1024] = {0};
	int r = 0;
	while (1) {
		r = read_nb_timeout(1, buf, sizeof(buf), 5);
		if (r <= 0) {
			time_t now;
			time(&now);
			struct tm *p = localtime(&now);
			printf("%d-%d-%d %d:%d:%d Error: read timeout\n", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
		} else {
			printf(">>> %s\n", buf);
			memset(buf, 0, sizeof(buf));
		}
	}	

	return 0;
}

