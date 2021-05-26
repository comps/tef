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

// in case we get interrupted by a signal
static int intr_safe_setlkw(int fd, struct flock *f)
{
    while (fcntl(fd, F_SETLKW, f) == -1) {
        if (errno != EINTR)
            return -1;
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

// TODO: write:
//        - coloring prefix
//        - the actual status
//        - coloring postfix
//        - space
//        - TEF_PREFIX
//        - newline
//       all in one writev(2), it is POSIX


// TODO: maybe rewrite this to writev(2), though dealing with its partial writes
//       (less than full bytes returned) across buffers would be really ugly
ssize_t write_safe(int fd, const void *buf, size_t count)
{
    size_t rc, written = 0;
    while (written < count) {
        if ((rc = write(fd, buf, count)) == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        written += rc;
    }
    return written;
}

#define CLPASS "\e[1;32m"
#define CLFAIL "\e[1;31m"
#define CLRUN  "\e[1;34m"
#define CLMARK "\e[1;90m"
#define CLRESET "\e[0m"
// avoid strlen
//#define CLLEN 7
#define CLLEN sizeof(CLPASS)-1
//#define CLRESETLEN 4
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
    if (line == NULL)
        return NULL;

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

bool tef_report(char *status, char *name)
{
    size_t len;
    char *line;
    if (is_terminal(0)) {
        line = format_line(status, name, &len, CLPASS);
        if (line) {
            write(1, line, len);
            free(line);
        }
        // color printout for known statuses
        // else black&white printout for unknown ones
    } else {
        // black&white printout
    }

    return true;
}

int main(int argc, char **argv)
{
    if (argc < 3)
        return 1;
    tef_report(argv[1], argv[2]);
    return 0;
}


#if 0
# log a status for a test name (path)
_tef_log_status() {
    local status="$1" name="$2"

    if [ -t 0 ]; then
        case "$status" in
            PASS) echo -ne "\033[1;32m${status}\033[0m" >&0 ;;
            FAIL) echo -ne "\033[1;31m${status}\033[0m" >&0 ;;
            RUN) echo -ne "\033[1;34m${status} \033[0m" >&0 ;;
            MARK) echo -ne "\033[1;90m${status}\033[0m" >&0 ;;
            *) echo -n "$status" >&0 ;;
        esac
        echo " $TEF_PREFIX/$name" >&0
    else
        echo "$status $TEF_PREFIX/$name" 2>/dev/null >&0
    fi

    if [ -n "$TEF_RESULTS_FD" ]; then
        echo "$status $TEF_PREFIX/$name" 2>/dev/null >&"$TEF_RESULTS_FD"
    fi

    return 0
}
#endif


