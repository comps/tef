#ifndef _TEF_HELPERS_H
#define _TEF_HELPERS_H

#define _STRINGIFY_INDIRECT(x) #x
#define _STRINGIFY(x) _STRINGIFY_INDIRECT(x)
#define _FILELINE __FILE__ ":" _STRINGIFY(__LINE__) ": "

#define ERROR_FD 2

// malloc-less, for low-resource fast error printing
#define PERROR(msg) perror_fd(ERROR_FD, __func__, _FILELINE, msg)
extern void perror_fd(int fd, const char *func, char *fileline, char *msg);

// dprintf-based, prefix msg with details
#define ERROR_FMT(fmt, ...) \
    dprintf(ERROR_FD, "tef error in %s@" _FILELINE fmt, \
            __func__, __VA_ARGS__)

// dprintf-based, like ERROR_FMT, but appends strerror(errno)
#define PERROR_FMT(fmt, ...) \
    dprintf(ERROR_FD, "tef error in %s@" _FILELINE fmt ": %s\n", \
            __func__, __VA_ARGS__, strerror(errno))

extern void *realloc_safe(void *ptr, size_t size);

#endif
