#include <string.h>

#if 0
// write directly to STDERR_FILENO, thread-safely, malloc-less
int perror_safe(const char *msg)
{
    char buf[128];
    int eno = errno;

    strcpy(buf, "error: ");
    strncpy(buf+7, msg, 128-8);

    size_t used = strlen(buf);
    char *err_msg = buf + used;
    size_t err_maxlen = sizeof(buf) - used - 1;

#if (_POSIX_C_SOURCE >= 200112L) && ! _GNU_SOURCE
    ...;
#else
    char *err = strerror_r(eno, err_msg, err_maxlen);
    // GNU version can store it in buf or elsewhere
    if (err != err_msg) {
        size_t err_len = strlen(err_msg);
        if (err_len < err_maxlen)
            err .......;
        err_maxlen = err_len < err_maxlen ? err_len : err_maxlen
        memmove(err_msg, err, strlen(err_msg));
#endif
}
#endif

