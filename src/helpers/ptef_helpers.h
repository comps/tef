#ifndef _PTEF_HELPERS_H
#define _PTEF_HELPERS_H

#define STRINGIFY_INDIRECT(x) #x
#define STRINGIFY(x) STRINGIFY_INDIRECT(x)
#define FILELINE __FILE__ ":" STRINGIFY(__LINE__) ": "

// shared across library/function calls
#define DEFAULT_ERROR_FD 2
extern __thread int current_error_fd;

// malloc-less, for low-resource fast error printing
#define PERROR(msg) perror_fd(current_error_fd, __func__, FILELINE, msg)
void perror_fd(int fd, const char *func, char *fileline, char *msg);

// dprintf-based, prefix msg with details
#define ERROR_FMT(fmt, ...) \
    do { \
        int orig_errno = errno; \
        dprintf(current_error_fd, "ptef error in %s@" FILELINE fmt, \
                __func__, __VA_ARGS__); \
        errno = orig_errno; \
    } while (0)

// dprintf-based, like ERROR_FMT, but appends strerror(errno)
#define PERROR_FMT(fmt, ...) \
    do { \
        int orig_errno = errno; \
        dprintf(current_error_fd, "ptef error in %s@" FILELINE fmt ": %s\n", \
                __func__, __VA_ARGS__, strerror(errno)); \
        errno = orig_errno; \
    } while (0)

void *realloc_safe(void *ptr, size_t size);

void *memcpy_append(void *dest, void *src, size_t n);

#endif
