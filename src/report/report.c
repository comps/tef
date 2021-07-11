#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ptef_helpers.h>

// in case we get interrupted by a signal
static int intr_safe_setlkw(int fd, struct flock *f)
{
    while (fcntl(fd, F_SETLKW, f) == -1) {
        if (errno != EINTR) {
            PERROR_FMT("fcntl(%d, F_SETLWK)", fd);
            return -1;
        }
    }
    return 0;
}
static int lock(int fd)
{
    struct flock f = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
    };
    return intr_safe_setlkw(fd, &f);
}
static int unlock(int fd)
{
    struct flock f = {
        .l_type = F_UNLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
    };
    return intr_safe_setlkw(fd, &f);
}

static bool is_terminal(int fd)
{
    struct termios tos;
    return !tcgetattr(fd, &tos);
}

static ssize_t write_safe_locked(int fd, const void *buf, size_t count)
{
    int write_errno;
    ssize_t rc;
    if (lock(fd) == -1)
        return -1;
    if ((rc = write_safe(fd, buf, count)) == -1)
        PERROR_FMT("write_safe(%d, ..)", fd);
    write_errno = errno;
    unlock(fd);
    errno = write_errno;
    return rc;
}

// allocate and return a line buffer
static char *format_line(char *status, char *name, size_t *len)
{
    size_t status_len = strlen(status);
    size_t name_len = strlen(name);

    // status, ' ', name, '\n', '\0'
    size_t line_len = status_len + name_len + 3;

    char *ptef_prefix = getenv_defined("PTEF_PREFIX");
    size_t ptef_prefix_len = 0;
    if (ptef_prefix) {
        ptef_prefix_len = strlen(ptef_prefix);
        line_len += ptef_prefix_len + 1;  // incl. '/' suffix
    } else {
        line_len += 1;  // just the leading '/'
    }

    char *line = malloc(line_len);
    if (line == NULL) {
        PERROR("malloc");
        return NULL;
    }

    char *part = line;
    part = memcpy_append(part, status, status_len);
    *part++ = ' ';
    if (ptef_prefix)
        part = memcpy_append(part, ptef_prefix, ptef_prefix_len);
    *part++ = '/';
    part = memcpy_append(part, name, name_len);
    *part++ = '\n';
    *part = '\0';

    *len = line_len-1;  // without '\0'
    return line;
}

#define CLGREEN "\e[1;32m"
#define CLRED   "\e[1;31m"
#define CLBLUE  "\e[1;34m"
#define CLGRAY  "\e[1;90m"
#define CLRESET "\e[0m"
static char *status_rewrites[][2] = {
    { "PASS", CLGREEN "PASS" CLRESET },
    { "FAIL", CLRED "FAIL" CLRESET },
    { "RUN", CLBLUE "RUN" CLRESET " " },
    { "MARK", CLGRAY "MARK" CLRESET },
};
#define REWRITES_CNT sizeof(status_rewrites)/sizeof(*status_rewrites)

// don't use STDOUT_FILENO as the standard specifies numbers
#define TERMINAL_FD 1

__asm__(".symver ptef_report_v0, ptef_report@@VERS_0");
__attribute__((used))
int ptef_report_v0(char *status, char *name)
{
    char *status_pretty = status;

    // if fd 1 is terminal, look up status color
    // if not or if status is unknown, color remains NULL --> no color codes
    bool isterm = is_terminal(TERMINAL_FD);
    if (isterm) {
        for (unsigned long i = 0; i < REWRITES_CNT; i++) {
            if (strcmp(status_rewrites[i][0], status)==0) {
                status_pretty = status_rewrites[i][1];
                break;
            }
        }
    }

    // write to stdout
    size_t len;
    char *line = format_line(status_pretty, name, &len);
    if (!line)
        goto err;

    if (write_safe_locked(TERMINAL_FD, line, len) == -1)
        goto err;

    // duplicate write to PTEF_RESULTS_FD
    char *ptef_results_fd = getenv_defined("PTEF_RESULTS_FD");
    if (ptef_results_fd) {
        int fd = atoi(ptef_results_fd);
        if (fd > 0) {
            if (status_pretty == status) {
                // black'n'white line already formatted
                if (write_safe_locked(fd, line, len) == -1)
                    goto err;
            } else {
                free(line);
                line = format_line(status, name, &len);
                if (!line)
                    goto err;
                if (write_safe_locked(fd, line, len) == -1)
                    goto err;
            }
        }
    }

    free(line);
    return 0;

err:
    free(line);
    return -1;
}
