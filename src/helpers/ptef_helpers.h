#ifndef _PTEF_HELPERS_H
#define _PTEF_HELPERS_H

#define STRINGIFY_INDIRECT(x) #x
#define STRINGIFY(x) STRINGIFY_INDIRECT(x)
#define FILELINE __FILE__ ":" STRINGIFY(__LINE__) ": "

#define DEFAULT_ERROR_FD 2
#define ERROR_FD DEFAULT_ERROR_FD

// malloc-less, for low-resource fast error printing
#define PERROR(msg) perror_fd(ERROR_FD, __func__, FILELINE, msg)
void perror_fd(int fd, const char *func, char *fileline, char *msg);

// dprintf-based, prefix msg with details
#define ERROR_FMT(fmt, ...) \
    dprintf(ERROR_FD, "ptef error in %s@" FILELINE fmt, \
            __func__, __VA_ARGS__)

// dprintf-based, like ERROR_FMT, but appends strerror(errno)
#define PERROR_FMT(fmt, ...) \
    dprintf(ERROR_FD, "ptef error in %s@" FILELINE fmt ": %s\n", \
            __func__, __VA_ARGS__, strerror(errno))

void *realloc_safe(void *ptr, size_t size);

void *memcpy_append(void *dest, void *src, size_t n);

#endif
