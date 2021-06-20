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

#include <ptef.h>
#include <ptef_helpers.h>

#include "common.h"

// retry on EINTR
static pid_t waitpid_safe(pid_t pid, int *wstatus, int options)
{
    pid_t ret;
    while ((ret = waitpid(pid, wstatus, options)) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return ret;
}

static _Noreturn void execute_child(char **argv, char *dir)
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
        char *ptef_prefix = getenv("PTEF_PREFIX");
        if (!ptef_prefix)
            ptef_prefix = "";

        char *pos;

        // add the current testname (subdir name) to PTEF_PREFIX
        testname = dir;
        size_t testname_len = strlen(testname);
        size_t ptef_prefix_len = strlen(ptef_prefix);
        // +2 for '/' and '\0'
        if ((tmp = malloc(ptef_prefix_len+testname_len+2)) == NULL) {
            PERROR("malloc");
            goto err;
        }
        pos = memcpy_append(tmp, ptef_prefix, ptef_prefix_len);
        *pos++ = '/';
        pos = memcpy_append(pos, testname, testname_len);
        *pos = '\0';
        if (setenv("PTEF_PREFIX", tmp, 1) == -1) {
            PERROR("setenv(PTEF_PREFIX, ..)");
            goto err;
        }

        // if PTEF_LOGS has relative path, prepend '../'
        char *ptef_logs = getenv("PTEF_LOGS");
        if (ptef_logs && *ptef_logs != '\0' && *ptef_logs != '/') {
            free(tmp);
            size_t ptef_logs_len = strlen(ptef_logs);
            // +4 for '../' and '\0'
            if ((tmp = malloc(ptef_logs_len+4)) == NULL) {
                PERROR("malloc");
                goto err;
            }
            pos = memcpy_append(tmp, "../", 3);
            pos = memcpy_append(pos, ptef_logs, ptef_logs_len);
            *pos = '\0';
            if (setenv("PTEF_LOGS", tmp, 1) == -1) {
                PERROR("setenv(PTEF_LOGS, ..)");
                goto err;
            }
        }
    } else {
        testname = argv[0];
    }

    // open a log file, duplicate it to "stderr"
    if ((logfd = ptef_mklog(testname)) == -1) {
        PERROR_FMT("ptef_mklog(%s)", testname);
        goto err;
    }
    if (dup2(logfd, DEFAULT_ERROR_FD) == -1) {
        PERROR_FMT("dup2(%d," STRINGIFY(DEFAULT_ERROR_FD) ")", logfd);
        goto err;
    }
    close(logfd);
    logfd = -1;

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
    // so fall through to 'err' and exit with failure
err:
    free(tmp);
    close(logfd);
    close(errout);
    exit(1);
#undef ERROR_FD
#define ERROR_FD DEFAULT_ERROR_FD
}

static int start_job(pid_t pid, char *name, struct exec_state *state)
{
    if (ptef_report("RUN", name) == -1)
        return -1;
    // find a (guaranteed) free slot in pid-to-name map and use it up
    struct pid_to_name *map = state->map;
    int i;
    for (i = 0; map[i].pid != -1; i++);
    map[i].pid = pid;
    map[i].name = name;
    state->running_jobs++;
    return 0;
}
static int finish_job(pid_t pid, struct exec_state *state, int exitcode)
{
    struct pid_to_name *map = state->map;
    // since we always use the first found '-1', it is guaranteed that we'll
    // find our pid on idx < running_jobs, or not at all
    int i, maxjobs = state->max_jobs;
    for (i = 0; i < maxjobs && map[i].pid != pid; i++);
    if (i >= maxjobs) {
        ERROR_FMT("pid %d not ours", pid);
        // technically not our problem, we can just skip it
        return 0;
    }
    char *status = exitcode == 0 ? "PASS" : "FAIL";
    int report_rc = ptef_report(status, map[i].name);
    // TODO: won't be needed once free() guarantees no errno changes
    int report_errno = errno;
    // reset the entry
    free(map[i].name);
    map[i].pid = -1;
    state->running_jobs--;
    errno = report_errno;
    return report_rc;
}

// allocate enough for exec_state itself + for all the pid-to-name
// mappings we'll ever need (there won't be more than opts->jobs children)
struct exec_state *create_exec_state(struct ptef_runner_opts *opts)
{
    struct exec_state *state;
    state = malloc(sizeof(struct exec_state)
                   + sizeof(struct pid_to_name) * opts->jobs);
    if (state != NULL) {
        state->max_jobs = opts->jobs;
        state->running_jobs = 0;
        struct pid_to_name empty = { -1, NULL };
        for (int i = 0; i < state->max_jobs; i++) {
            memcpy(&state->map[i], &empty, sizeof(empty));
        }
    }
    return state;
}
int destroy_exec_state(struct exec_state *state)
{
    int ret = 0;
    pid_t child;
    int wstatus;
    // finish all jobs
    while (state->running_jobs-- > 0) {
        if ((child = waitpid_safe(-1, &wstatus, 0)) > 0) {
            // yes, this can repeatedly call finish_job() on error,
            // but what's the worst that can happen? .. we'd need to
            // collect all children later in this func anyway
            if (finish_job(child, state, WEXITSTATUS(wstatus)) == -1)
                ret = -1;
        } else {
            PERROR("waitpid");
            ret = -1;
        }
    }
    free(state);
    return ret;
}

// execute an executable file in CWD or a directory with an executable file
// named after basename
// - argv must have [0] allocated and unused (to be used for argv[0])
//   and be terminated at [1] or later with NULL
int execute(char *file, enum exec_entry_type typehint, char **argv,
            struct ptef_runner_opts *opts, struct exec_state *state)
{
    if (typehint == EXEC_TYPE_UNKNOWN) {
        if (fstatat_type(AT_FDCWD, file, &typehint) == -1) {
            PERROR_FMT("fstatat %s", file);
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

    // reap any zombies that already exited between execute() calls
    pid_t child;
    int wstatus;
    while ((child = waitpid_safe(-1, &wstatus, WNOHANG)) > 0)
        if (finish_job(child, state, WEXITSTATUS(wstatus)) == -1)
            return -1;
    if (child == -1 && errno != ECHILD) {
        PERROR("waitpid WNOHANG");
        return -1;
    }

    // duplicate filename for storage in pid<->testname map array
    // this belongs to start_job(), but we don't want to deal with a fork()ed
    // child if this fails, so do it early
    char *name_duped;
    if ((name_duped = strdup(file)) == NULL) {
        PERROR("strdup");
        return -1;
    }

    child = fork();
    switch (child) {
        case -1:
            return -1;
        case 0:
            execute_child(argv, child_dir);
            break;
        default:
            if (start_job(child, name_duped, state) == -1)
                return -1;
            break;
    }

    // if we're at maximum of running jobs, block until one exits
    if (state->running_jobs >= state->max_jobs) {
        if ((child = waitpid_safe(-1, &wstatus, 0)) > 0) {
            if (finish_job(child, state, WEXITSTATUS(wstatus)) == -1)
                return -1;
        } else {
            PERROR("waitpid");
            return -1;
        }
    }

    return 0;
}
