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

void execute_file(char *exe, char **argv)
{
    // don't touch TEF_PREFIX
    // do execv()
    (void)exe;
    (void)argv;
}

void execute_dir(char *exe, char **argv, char *dir)
{
    // append 'dir' to TEF_PREFIX, do setenv, check for err
    // - if getenv(TEF_PREFIX) is NULL, create it
    // if TEF_LOGS exists and first char isn't '/', prepend '../' + setenv
    // do chdir(dir)
    // do execv()
    (void)exe;
    (void)argv;
    (void)dir;
}

static pid_t waitpid_safe(pid_t pid, int *wstatus, int options)
{
    pid_t ret;
    while ((ret = waitpid(pid, wstatus, options)) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return ret;
}

// execute an executable file in CWD or a directory with an executable file
// named after basename
// - argv must have [0] allocated and unused (to be used for argv[0])
//   and be terminated at [1] or later with NULL
int
execute(char *exe, enum exec_entry_type typehint, char **argv,
        struct tef_runner_opts *opts, struct exec_state *state)
{
    if (typehint == EXEC_TYPE_UNKNOWN) {
        if (fstatat_type(AT_FDCWD, exe, &typehint) == -1) {
            ERROR("fstatat");
            //state->failed = true;
            return -1;
        }
    }

    // prepare env, TEF_LOGS, TEF_PREFIX
    //  - must be done inside child, because parent might need
    //    the current value of TEF_PREFIX after we exit from
    //    the runner func

    // reap any zombies that already exited
    pid_t child;
    int wstatus;
    while ((child = waitpid_safe(-1, &wstatus, WNOHANG)) > 0) {
        state->running_jobs--;
        if (WEXITSTATUS(child) != 0)
            state->failed = true;
    }
    if (child == -1) {
        ERROR("waitpid WNOHANG");
        return -1;
    }

    char *basename = opts->argv0;

    switch (fork()) {
        case -1:
            return -1;
        case 0:

            // dup() stderr, so we can still write errors to original stderr
            // even after we close() and open() it in a mklog-created file

            // mklog where ???
            // - both file and dir will have .log for the current level,
            //   so maybe here
            // - new subdir can be caught by execute_dir() above modifying
            //   TEF_LOGS, so the next invocation in a new subrunner creates
            //   the directory

            switch (typehint) {
                case EXEC_TYPE_FILE:
                    argv[0] = exe;
                    //debug
                    printf("executing file %s:", exe);
                    for (int i = 0; argv[i] != NULL; i++) {
                        printf(" %s", argv[i]);
                    }
                    putchar('\n');
                    //debug end
                    break;
                case EXEC_TYPE_DIR:
                    argv[0] = basename;
                    printf("executing dir %s:", exe);
                    //debug
                    for (int i = 0; argv[i] != NULL; i++) {
                        printf(" %s", argv[i]);
                    }
                    putchar('\n');
                    //debug end
                    break;
                default:
                    ERROR_FORMAT("invalid exec type %d\n", typehint);
                    return -1;
            }
            // ..
        default:
            break;
    }



    // if execution succeded (not returned yet):
    // - increase state->running_jobs by 1

    // if we're at maximum of running jobs, block until one exits
    if (state->running_jobs >= opts->jobs) {
        child = waitpid_safe(-1, &wstatus, 0);
        if (child > 0) {
            state->running_jobs--;
            if (WEXITSTATUS(child) != 0)
                state->failed = true;
        } else {
            ERROR("waitpid");
            return -1;
        }
    }

    return 0;
}
