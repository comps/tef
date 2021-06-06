#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>

// malloc-less verbose perror()
void perror_fd(int fd, const char *func, char *fileline, char *msg)
{
    char prefix[] = "tef error in ";
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
}

// free the block if realloc fails
void *realloc_safe(void *ptr, size_t size)
{
    void *new = realloc(ptr, size);
    if (new == NULL)
        free(ptr);
    return new;
}

// like stpcpy, but without the repeated internal strlen()
void *memcpy_append(void *dest, void *src, size_t n)
{
    return memcpy(dest, src, n) + n;
}
