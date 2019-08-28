#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


static time_t last_read_time;
static time_t last_write_time;

/* Do a safe read, handling any needed looping and error handling.
 * Returns the count of the bytes read, which will only be different
 * from "len" if we encountered an EOF.  This routine is not used on
 * the socket except very early in the transfer. 
 * 
 * Return:
 *     -2: timeout
 *     -1: fail, and errno is set appropriately
 * */
size_t safe_read(int fd, char *buf, size_t len, int timeout_seconds)
{
    size_t got = 0;

    while (1) {
        struct timeval tv;
        fd_set r_fds, e_fds;
        int cnt;

        FD_ZERO(&r_fds);
        FD_SET(fd, &r_fds);
        FD_ZERO(&e_fds);
        FD_SET(fd, &e_fds);

        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;

        cnt = select(fd+1, &r_fds, NULL, &e_fds, &tv);
        if (cnt <= 0) {
            if (cnt < 0 && errno == EBADF) {
                log_error("safe_read select failed(%d):%s (%s)", errno, strerror(errno), who_am_i(fd));
                return -1;
            }
            
            // check timeout
            time_t t = time(NULL);
            if (t - last_read_time > timeout_seconds) {
                log_error("io timeout after %d seconds", (int)(t - last_read_time));
                return -2;
            }

            continue;
        }

        if (FD_ISSET(fd, &r_fds)) {
            int n = read(fd, buf + got, len - got);
            last_read_time = time(NULL);
            if (n == 0)
                break;
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                log_error("failed to read %ld bytes (%s)", (long)len, who_am_i(fd));
                return -1;
            }
            if ((got += (size_t)n) == len)
                break;
        }
    }

    return got;
}


/* Do a safe write, handling any needed looping and error handling.
 * Returns only if everything was successfully written.  This routine
 * is not used on the socket except very early in the transfer. 
 * 
 * Return:
 *     -2: timeout
 *     -1: fail, and errno is set appropriately
 * 
 * */
size_t safe_write(int fd, const char *buf, size_t len, int timeout_seconds)
{
    int n;

    n = write(fd, buf, len);
    last_write_time = time(NULL);
    if ((size_t)n == len)
        return n;

    if (n < 0) {
        if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN) {
          write_failed:
            log_error("safe_write failed(%d):%s to write %ld bytes to %s (%s)",
                    errno, strerror(errno), (long)len, what_fd_is(fd), who_am_i(fd));
            //exit_cleanup(11);    
            return -1;
        }
    } else {
        buf += n;
        len -= n;
    }
    
    while (len) {
        struct timeval tv;
        fd_set w_fds;
        int cnt;

        FD_ZERO(&w_fds);
        FD_SET(fd, &w_fds);
        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;

        cnt = select(fd + 1, NULL, &w_fds, NULL, &tv);
        if (cnt <= 0) {
            if (cnt < 0 && errno == EBADF) {
                log_error("safe_write select failed(%d):%s on %s (%s)",
                        errno, strerror(errno), what_fd_is(fd), who_am_i(fd));
                //exit_cleanup(errno);
                return -1;
            }
            
            // check timeout
            time_t t = time(NULL);
            if (t - last_write_time > timeout_seconds) {
                log_error("io timeout after %d seconds", (int)(t - last_write_time));
                return -2;
            }
            
            continue;
        }
        
        if (FD_ISSET(fd, &w_fds)) {
            n = write(fd, buf, len);
            last_write_time = time(NULL);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                goto write_failed;
            }
            buf += n;
            len -= n;
        }
    }  // end while
}
