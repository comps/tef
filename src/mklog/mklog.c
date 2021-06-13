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

// rotate old logs, create new one, return its fd
static int open_log(int dirfd, char *testname)
{
    // do log rotation (unlinkat(), renameat()) based on this dir fd
    // create the final new logfile via open() O_CREAT | O_WRONLY
    // return it as open fd
    char tmpl_suffix[] = ".1.log";

    size_t testname_len = strlen(testname);

    char *from = NULL, *to = NULL;

    if ((from = malloc(testname_len+sizeof(tmpl_suffix))) == NULL) {
        PERROR("malloc");
        goto err;
    }

    char *pos = from;
    pos = memcpy_append(pos, testname, testname_len);
    pos = memcpy_append(pos, tmpl_suffix, sizeof(tmpl_suffix));

    if ((to = strdup(from)) == NULL) {
        PERROR("strdup");
        goto err;
    }

    // offsets of the single-digit logrotated number
    char *fromnr = from+testname_len+1;
    char *tonr = to+testname_len+1;

    // unlink (remove) the oldest log if it exists
    *fromnr = '9';
    if (unlinkat(dirfd, from, 0) == -1 && errno != ENOENT) {
        PERROR_FMT("unlinkat %s", from);
        goto err;
    }

    // rotate everything one iteration
    // unfortunately this is the easiest way without expensive printf
#define MOVE(fromnum,tonum) \
    do { \
        *fromnr = fromnum; \
        *tonr = tonum; \
        if (renameat(dirfd, from, dirfd, to) == -1 && errno != ENOENT) { \
            PERROR_FMT("rename %s to %s", from, to); \
            goto err; \
        } \
    } while (0)
    MOVE('8','9');
    MOVE('7','8');
    MOVE('6','7');
    MOVE('5','6');
    MOVE('4','5');
    MOVE('3','4');
    MOVE('2','3');
    MOVE('1','2');
#undef MOVE

    // manually move .log to .1.log here
    // re-use the already-allocated 'to' by replacing '.2.log' with '.log'
    // 'from' already has '.1.log' here
    strcpy(to+testname_len, ".log");
    if (renameat(dirfd, to, dirfd, from) == -1 && errno != ENOENT) {
        PERROR_FMT("rename %s to %s", to, from);
        goto err;
    }

    // create and open a new log
    int fd = openat(dirfd, to, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        PERROR_FMT("openat(..,%s,O_CREAT|O_WRONLY)", to);
        goto err;
    }

    free(from);
    free(to);
    return fd;
err:
    free(from);
    free(to);
    return -1;
}

// recursive mkdir start at srcfd (O_DIRECTORY)
static int mkpath(int srcfd, char *path)
{
    char *buff;
    if ((buff = malloc(strlen(path)+1)) == NULL) {
        PERROR("malloc");
        return -1;
    }

    char *pos = buff;
    char *start = path, *end = path;
    while ((end = strchr(start, '/')) != NULL) {
        if (end == start) {
            start++;
            continue;
        }
        pos = memcpy_append(pos, start, end-start+1);
        *pos = '\0';
        if (mkdirat(srcfd, buff, 0755) == -1 && errno != EEXIST) {
            PERROR_FMT("mkdirat %s", buff);
            goto err;
        }
        start = end+1;
    }
    if (*start != '\0') {
        strcpy(pos, start);
        if (mkdirat(srcfd, buff, 0755) == -1 && errno != EEXIST) {
            PERROR_FMT("mkdirat %s", buff);
            goto err;
        }
    }

    free(buff);
    return 0;
err:
    free(buff);
    return -1;
}

static int open_create_dir(char *name)
{
    int fd = open(name, O_DIRECTORY);
    if (fd == -1) {
        if (errno != ENOENT) {
            PERROR_FMT("open %s", name);
            goto err;
        }
        if (mkdir(name, 0755) == -1) {
            PERROR_FMT("mkdir %s", name);
            goto err;
        }
        if ((fd = open(name, O_DIRECTORY)) == -1) {
            PERROR_FMT("open %s", name);
            goto err;
        }
    }
    return fd;
err:
    close(fd);
    return -1;
}

// open/create TEF_LOGS, create path inside it, open final leaf dir
static int open_tef_logs(char *tef_logs, char *tef_prefix)
{
    char *combined = NULL;

    // try opening it, create if necessary
    int logsfd = open_create_dir(tef_logs);
    if (logsfd == -1)
        goto err;

    if (!tef_prefix)
        tef_prefix = "";

    // 'mkdir -p' any directories inside TEF_LOGS
    if (mkpath(logsfd, tef_prefix) == -1)
        goto err;

    size_t tef_logs_len = strlen(tef_logs);
    size_t tef_prefix_len = strlen(tef_prefix);

    // concat TEF_LOGS + '/' + TEF_PREFIX + '\0'
    combined = malloc(tef_logs_len+tef_prefix_len+2);
    if (combined == NULL) {
        PERROR("malloc");
        goto err;
    }

    char *pos = combined;
    pos = memcpy_append(pos, tef_logs, tef_logs_len);
    *pos++ = '/';
    pos = memcpy_append(pos, tef_prefix, tef_prefix_len);
    *pos = '\0';

    // open the concatenated result
    close(logsfd);
    logsfd = open(combined, O_DIRECTORY);
    if (logsfd == -1) {
        PERROR_FMT("open %s", combined);
        goto err;
    }

    free(combined);
    return logsfd;
err:
    free(combined);
    close(logsfd);
    return -1;
}

// create and open a log file for testname, return its fd
__asm__(".symver tef_mklog_v0, tef_mklog@@VERS_0");
int tef_mklog_v0(char *testname)
{
    int logsfd;

    char *tef_logs = getenv("TEF_LOGS");
    if (tef_logs && *tef_logs != '\0')
        logsfd = open_tef_logs(tef_logs, getenv("TEF_PREFIX"));
    else
        logsfd = open_create_dir("logs");

    if (logsfd == -1)
        goto err;

    int fd = open_log(logsfd, testname);
    close(logsfd);
    return fd;
err:
    close(logsfd);
    return -1;
}