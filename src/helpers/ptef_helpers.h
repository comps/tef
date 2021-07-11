#ifndef _PTEF_HELPERS_H
#define _PTEF_HELPERS_H

#define STRINGIFY_INDIRECT(x) #x
#define STRINGIFY(x) STRINGIFY_INDIRECT(x)
#define FILELINE __FILE__ ":" STRINGIFY(__LINE__) ": "

#define DEFAULT_ERROR_FD 2
#define DEFAULT_ERROR_FD_STR STRINGIFY(DEFAULT_ERROR_FD)

// malloc-less, for low-resource fast error printing
void perror_fd(int fd, const char *func, char *fileline, char *msg);
#define PERROR_FD(fd, msg) perror_fd(fd, __func__, FILELINE, msg)
#define PERROR(msg) PERROR_FD(DEFAULT_ERROR_FD, msg)

// dprintf-based, prefix msg with details
#define ERROR_FMT_FD(fd, fmt, ...) \
    do { \
        int orig_errno = errno; \
        dprintf(fd, "ptef error in %s@" FILELINE fmt, \
                __func__, __VA_ARGS__); \
        errno = orig_errno; \
    } while (0)
#define ERROR_FMT(fmt, ...) ERROR_FMT_FD(DEFAULT_ERROR_FD, fmt, __VA_ARGS__)

// dprintf-based, like ERROR_FMT, but appends strerror(errno)
#define PERROR_FMT_FD(fd, fmt, ...) \
    do { \
        int orig_errno = errno; \
        dprintf(fd, "ptef error in %s@" FILELINE fmt ": %s\n", \
                __func__, __VA_ARGS__, strerror(errno)); \
        errno = orig_errno; \
    } while (0)
#define PERROR_FMT(fmt, ...) PERROR_FMT_FD(DEFAULT_ERROR_FD, fmt, __VA_ARGS__)

void *realloc_safe(void *ptr, size_t size);

ssize_t write_safe(int fd, const void *buf, size_t count);

char *getenv_defined(const char *name);

void *memcpy_append(void *dest, void *src, size_t n);

#endif
