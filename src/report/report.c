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

#include <tef_helpers.h>

// in case we get interrupted by a signal
static int intr_safe_setlkw(int fd, struct flock *f)
{
    while (fcntl(fd, F_SETLKW, f) == -1) {
        if (errno != EINTR) {
            ERROR("fcntl(F_SETLWK)");
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

// TODO: maybe rewrite this to writev(2), though dealing with its partial writes
//       (less than full bytes returned) across buffers would be really ugly
static ssize_t write_safe(int fd, const void *buf, size_t count)
{
    ssize_t rc, written = 0;
    while ((size_t)written < count) {
        if ((rc = write(fd, buf, count)) == -1) {
            if (errno == EINTR)
                continue;
            ERROR("write");
            return -1;
        }
        written += rc;
    }
    return written;
}
static ssize_t write_safe_locked(int fd, const void *buf, size_t count)
{
    int write_errno;
    ssize_t rc;
    if (lock(fd) == -1)
        return -1;
    rc = write_safe(fd, buf, count);
    write_errno = errno;
    unlock(fd);
    errno = write_errno;
    return rc;
}

#define CLGREEN "\e[1;32m"
#define CLRED   "\e[1;31m"
#define CLBLUE  "\e[1;34m"
#define CLGRAY  "\e[1;90m"
#define CLRESET "\e[0m"
// avoid strlen
#define CLLEN sizeof(CLGREEN)-1
#define CLRESETLEN sizeof(CLRESET)-1

static void *memcpy_append(void *dest, void *src, size_t n)
{
    return memcpy(dest, src, n) + n;
}
// allocate and return a line buffer
static char *format_line(char *status, char *name, size_t *len, char *color)
{
    size_t status_len = strlen(status);
    size_t name_len = strlen(name);

    size_t line_len;
    if (color) {
        // color, status, color rst, space, name, '\n', '\0'
        line_len = CLLEN + status_len + CLRESETLEN + name_len + 3;
    } else {
        // status, space, name, '\n', '\0'
        line_len = status_len + name_len + 3;
    }

    char *tef_prefix = getenv("TEF_PREFIX");
    size_t tef_prefix_len = 0;
    if (tef_prefix) {
        tef_prefix_len = strlen(tef_prefix);
        line_len += tef_prefix_len + 1;  // incl. '/' suffix
    }

    char *line = malloc(line_len);
    if (line == NULL) {
        ERROR("malloc");
        return NULL;
    }

    char *part = line;
    if (color) {
        part = memcpy_append(part, color, CLLEN);
        part = memcpy_append(part, status, status_len);
        part = memcpy_append(part, CLRESET, CLRESETLEN);
    } else {
        part = memcpy_append(part, status, status_len);
    }
    *part++ = ' ';
    if (tef_prefix) {
        part = memcpy_append(part, tef_prefix, tef_prefix_len);
        *part++ = '/';
    }
    part = memcpy_append(part, name, name_len);
    *part++ = '\n';
    *part = '\0';

    *len = line_len-1;  // without '\0'
    return line;
}

// status match, pretty name, color
static char *status_colors[][3] = {
    { "PASS", "PASS", CLGREEN },
    { "FAIL", "FAIL", CLRED   },
    { "RUN",  "RUN ", CLBLUE  },
    { "MARK", "MARK", CLGRAY  },
};
#define STATUSLEN sizeof(status_colors)/sizeof(*status_colors)

// don't use STDOUT_FILENO as the standard specifies numbers
#define TERMINAL_FD 1

__asm__(".symver tef_report_v0, tef_report@@VERS_0");
int tef_report_v0(char *status, char *name)
{
    char *status_pretty = status;
    char *color = NULL;

    // if fd 1 is terminal, look up status color
    // if not or if status is unknown, color remains NULL --> no color codes
    bool isterm = is_terminal(TERMINAL_FD);
    if (isterm) {
        for (unsigned long i = 0; i < STATUSLEN; i++) {
            if (strcmp(status_colors[i][0], status)==0) {
                status_pretty = status_colors[i][1];
                color = status_colors[i][2];
                break;
            }
        }
    }

    // write to stdout
    size_t len;
    char *line = format_line(status_pretty, name, &len, color);
    if (!line)
        goto err;

    if (write_safe_locked(TERMINAL_FD, line, len) == -1)
        goto err;

    // duplicate write to TEF_RESULTS_FD
    char *tef_results_fd = getenv("TEF_RESULTS_FD");
    if (tef_results_fd) {
        int fd = atoi(tef_results_fd);
        if (fd > 0) {
            if (!color) {
                // black'n'white line already formatted
                if (write_safe_locked(fd, line, len) == -1)
                    goto err;
            } else {
                free(line);
                line = format_line(status_pretty, name, &len, NULL);
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
