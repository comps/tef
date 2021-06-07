#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
//#include <dirent.h>
//#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <tef.h>
#include <tef_helpers.h>

#include "common.h"

static pid_t waitpid_safe(pid_t pid, int *wstatus, int options)
{
    pid_t ret;
    while ((ret = waitpid(pid, wstatus, options)) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return ret;
}

// return exit code for the child process
// or never return if execve() succeeds
static int execute_child(char **argv, char *dir)
{
    char *tmp = NULL;
    int logfd = -1;

    // save stderr so we can still report errors to original output
    int errout;
    if ((errout = dup(DEFAULT_ERROR_FD)) == -1) {
        PERROR("dup(" STRINGIFY(DEFAULT_ERROR_FD) ")");
        goto err;
    }
    if (close(DEFAULT_ERROR_FD) == -1) {
        PERROR("close(" STRINGIFY(DEFAULT_ERROR_FD) ")");
        goto err;
    }
#undef ERROR_FD
#define ERROR_FD errout

    char *testname;

    if (dir) {
        char *tef_prefix = getenv("TEF_PREFIX");
        if (!tef_prefix)
            tef_prefix = "";

        char *pos;

        // add the current testname (subdir name) to TEF_PREFIX
        testname = dir;
        size_t testname_len = strlen(testname);
        size_t tef_prefix_len = strlen(tef_prefix);
        // +2 for '/' and '\0'
        if ((tmp = malloc(tef_prefix_len+testname_len+2)) == NULL) {
            PERROR("malloc");
            goto err;
        }
        pos = memcpy_append(tmp, tef_prefix, tef_prefix_len);
        *pos++ = '/';
        pos = memcpy_append(pos, testname, testname_len);
        *pos = '\0';
        if (setenv("TEF_PREFIX", tmp, 1) == -1) {
            PERROR("setenv(TEF_PREFIX, ..)");
            goto err;
        }

        // if TEF_LOGS has relative path, prepend '../'
        char *tef_logs = getenv("TEF_LOGS");
        if (tef_logs && *tef_logs != '\0' && *tef_logs != '/') {
            free(tmp);
            size_t tef_logs_len = strlen(tef_logs);
            // +4 for '../' and '\0'
            if ((tmp = malloc(tef_logs_len+4)) == NULL) {
                PERROR("malloc");
                goto err;
            }
            pos = memcpy_append(tmp, "../", 3);
            pos = memcpy_append(pos, tef_logs, tef_logs_len);
            *pos = '\0';
            if (setenv("TEF_LOGS", tmp, 1) == -1) {
                PERROR("setenv(TEF_LOGS, ..)");
                goto err;
            }
        }
    } else {
        testname = argv[0];
    }

    if ((logfd = tef_mklog(testname)) == -1) {
        PERROR_FMT("tef_mklog(%s)", testname);
        goto err;
    }

    if (dir) {
        if (chdir(dir) == -1) {
            PERROR_FMT("chdir(%s)", dir);
            goto err;
        }
    }

#ifdef O_CLOEXEC
    // close our error-reporting channel on execve()
    if (fcntl(errout, F_SETFD, O_CLOEXEC) == -1) {
        PERROR("fcntl(.., F_SETFD, O_CLOEXEC)");
        goto err;
    }
#else
    // close it now + switch to test's stderr as the next best thing
    if (close(errout) == -1) {
        PERROR("close(errout)");
        goto err;
    }
#undef ERROR_FD
#define ERROR_FD DEFAULT_ERROR_FD
#endif

    // do execv()
    if (execv(argv[0], argv) == -1) {
        PERROR_FMT("execv(%s,..)", argv[0]);
        goto err;
    }

    // this shouldn't be reachable,
    // so fall through to 'err' and return failure
err:
    free(tmp);
    close(logfd);
    close(errout);
    return 1;
#undef ERROR_FD
#define ERROR_FD DEFAULT_ERROR_FD
}

// execute an executable file in CWD or a directory with an executable file
// named after basename
// - argv must have [0] allocated and unused (to be used for argv[0])
//   and be terminated at [1] or later with NULL
int execute(char *file, enum exec_entry_type typehint, char **argv,
            struct tef_runner_opts *opts, struct exec_state *state)
{
    if (typehint == EXEC_TYPE_UNKNOWN) {
        if (fstatat_type(AT_FDCWD, file, &typehint) == -1) {
            PERROR_FMT("fstatat %s", file);
            //state->failed = true;
            return -1;
        }
    }

    char *child_dir;
    switch (typehint) {
        case EXEC_TYPE_FILE:
            child_dir = NULL;
            argv[0] = file;
            break;
        case EXEC_TYPE_DIR:
            child_dir = file;
            argv[0] = opts->argv0;  // argv0 of the runner
            break;
        default:
            ERROR_FMT("invalid exec type %d\n", typehint);
            return -1;
    }

    // reap any zombies that already exited
    pid_t child;
    int wstatus;
    while ((child = waitpid_safe(-1, &wstatus, WNOHANG)) > 0) {
        state->running_jobs--;
        if (WEXITSTATUS(child) != 0)
            state->failed = true;
    }
    if (child == -1) {
        PERROR("waitpid WNOHANG");
        return -1;
    }

    switch (fork()) {
        case -1:
            return -1;
        case 0:
            exit(execute_child(argv, child_dir));
            break;
        default:
            state->running_jobs++;
            break;
    }

    // if we're at maximum of running jobs, block until one exits
    if (state->running_jobs >= opts->jobs) {
        child = waitpid_safe(-1, &wstatus, 0);
        if (child > 0) {
            state->running_jobs--;
            if (WEXITSTATUS(child) != 0)
                state->failed = true;
        } else {
            PERROR("waitpid");
            return -1;
        }
    }

    return 0;
}
