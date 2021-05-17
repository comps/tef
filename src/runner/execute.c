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

#include "execute.h"

int fstatat_type(int dirfd, char *pathname, enum exec_entry_type *type)
{
    int ret;
    struct stat statbuf;
    if ((ret = fstatat(dirfd, pathname, &statbuf, 0)) == -1)
        return -1;
    switch (statbuf.st_mode & S_IFMT) {
        case S_IFREG:
            *type = EXEC_TYPE_FILE;
            break;
        case S_IFDIR:
            *type = EXEC_TYPE_DIR;
            break;
        default:
            *type = EXEC_TYPE_INVALID;
            break;
    }
    return ret;
}

// execute an executable file in CWD or a directory with an executable file
// named after basename
// - if argv is NULL, pass no args, else argv must have [0] allocated and unused
//   (to be used for argv[0]) and be terminated at [1] or later with NULL
// TODO:
//    - execution logic - it needs a big 'if' between directory ents and CWD ents
//      as the directory ones will need chdir, etc.
//    - environment variable export (TEF_LOGS, etc.) before any forking/execution
//      - probably in parent chain somewhere, so we don't do it for every executable
//    - some child function for the execve
void
execute(char *exe, enum exec_entry_type typehint, char **argv,
        char *basename, struct exec_state *state)
{
    if (typehint == EXEC_TYPE_UNKNOWN) {
        if (fstatat_type(AT_FDCWD, exe, &typehint) == -1) {
            perror("execute fstatat");
            state->failed = true;
            return;
        }
    }

    switch (typehint) {
        case EXEC_TYPE_FILE:
            if (argv != NULL) {
                printf("executing file %s:", exe);
                argv[0] = basename;
                for (int i = 0; argv[i] != NULL; i++) {
                    printf(" %s", argv[i]);
                }
                putchar('\n');
            } else {
                printf("executing file %s (no args)\n", exe);
            }
            break;
        case EXEC_TYPE_DIR:
            if (argv != NULL) {
                printf("executing dir %s:", exe);
                argv[0] = basename;
                for (int i = 0; argv[i] != NULL; i++) {
                    printf(" %s", argv[i]);
                }
                putchar('\n');
            } else {
                printf("executing dir %s (no args)\n", exe);
            }
            break;
        default:
            fprintf(stderr, "cannot execute non-exec non-dir\n");
            return;
    }
}
