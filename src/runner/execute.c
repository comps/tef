#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

// execute an executable file in CWD or a directory with an executable file
// named after basename
// - argv must have [0] allocated and unused (to be used for argv[0])
//   and be terminated at [1] or later with NULL
void
execute(char *exe, enum exec_entry_type typehint, char **argv,
        struct tef_runner_opts *opts, struct exec_state *state)
{
    if (typehint == EXEC_TYPE_UNKNOWN) {
        if (fstatat_type(AT_FDCWD, exe, &typehint) == -1) {
            perror("execute fstatat");
            state->failed = true;
            return;
        }
    }

    // prepare env, TEF_LOGS, TEF_PREFIX

    // reap any zombies, waitpid(-1, &wstatus, WNOHANG)
    // - and process their exit code, potentially setting state->failed=true
    // - decrement state->running_jobs for each one reaped

    // use fork, clone is linux only

    char *basename = opts->argv0;

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
            fprintf(stderr, "cannot execute non-exec non-dir\n");
            return;
    }

    int max_jobs = opts->jobs;

    (void)max_jobs;

    // if execution succeded (not returned yet):
    // - increase state->running_jobs by 1
    // - if state->running_jobs is >= max_jobs
    //   - wait for any one child: waitpid(-1, &wstatus, 0)
    //     - and process status, maybe set state->failed=true
}
