#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "ptef_helpers.h"

__thread int current_error_fd = DEFAULT_ERROR_FD;

// malloc-less verbose perror()
void perror_fd(int fd, const char *func, char *fileline, char *msg)
{
    int orig_errno = errno;
    char prefix[] = "ptef error in ";
    char *err = strerror(errno);
    struct iovec iov[] = {
        { prefix, sizeof(prefix)-1 },
        { (void*)func, strlen(func) },
        { "@", 1 },
        { fileline, strlen(fileline) },
        { msg, strlen(msg) },
        { ": ", 2 },
        { err, strlen(err) },
        { "\n", 1 },
    };
    writev(fd, (struct iovec *)iov, sizeof(iov)/sizeof(*iov));
    errno = orig_errno;
}

// free the block if realloc fails
void *realloc_safe(void *ptr, size_t size)
{
    void *new = realloc(ptr, size);
    if (new == NULL)
        free(ptr);
    return new;
}

// re-try until the entire buffer is written
ssize_t write_safe(int fd, const void *buf, size_t count)
{
    ssize_t rc, written = 0;
    while ((size_t)written < count) {
        if ((rc = write(fd, buf, count)) == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        written += rc;
    }
    return written;
}

// like stpcpy, but without the repeated internal strlen()
void *memcpy_append(void *dest, void *src, size_t n)
{
    return memcpy(dest, src, n) + n;
}
