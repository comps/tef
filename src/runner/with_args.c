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

#include <ptef.h>
#include <ptef_helpers.h>

#include "common.h"

static char *sane_arg(char *a)
{
    while (*a == '/')
        a++;
    // empty is invalid
    if (a[0] == '\0')
        return NULL;
    // '.' or '..' are invalid
    // probably more efficient than 4 strcmp's
    if (a[0] == '.') {
        if (a[1] == '\0' || a[1] == '/')
            return NULL;
        if (a[1] == '.') {
            if (a[2] == '\0' || a[2] == '/')
                return NULL;
        }
    }
    return a;
}

int for_each_arg(int argc, char **argv, char *basename, int jobs)
{
    struct exec_state *state;
    if ((state = create_exec_state(jobs)) == NULL) {
        PERROR("create_exec_state");
        return -1;
    }

    // [0] is execute-reserved for argv[0],
    // [1] is the one non-merged arg
    // [2] is NULL terminator
    char *arg[3] = { NULL };

    char *prefix = NULL;
    for (int i = 0; i < argc; i++) {
        char *sane = sane_arg(argv[i]);
        if (!sane) {
            ERROR_FMT("arg failed sanity check: %s\n", argv[i]);
            continue;
        }

        char *slash = strchr(sane, '/');

        // standalone arg, just execute it without args
        if (!slash) {
            arg[1] = NULL;
            if (execute(sane, EXEC_TYPE_UNKNOWN, arg, basename, state) == -1)
                goto err;
            continue;
        }

        ptrdiff_t prefix_len = slash-sane;
        prefix = strndup(sane, prefix_len);
        if (prefix == NULL) {
            PERROR("strndup");
            goto err;
        }

        arg[1] = sane+prefix_len+1;  // skip prefix + '/'
        if (execute(prefix, EXEC_TYPE_UNKNOWN, arg, basename, state) == -1)
            goto err;

        free(prefix);
        prefix = NULL;
    }

    return destroy_exec_state(state);

err:
    free(prefix);
    destroy_exec_state(state);
    return -1;
}

int for_each_merged_arg(int argc, char **argv, char *basename, int jobs)
{
    char **merged;
    struct exec_state *state;

    if ((state = create_exec_state(jobs)) == NULL) {
        PERROR("create_exec_state");
        return -1;
    }

    // +2 is for argv[0] and terminating NULL ptr
    if ((merged = malloc((argc+2)*sizeof(argv))) == NULL) {
        PERROR("malloc");
        return -1;
    }
    // leave 0 for execve argv[0] being executable name
    int merged_idx = 1;
    // NULL-terminate, will be moved on each arg addition
    merged[merged_idx] = NULL;

    char *prefix = NULL;
    ptrdiff_t prefix_len = 0;

    for (int i = 0; i < argc; i++) {
        char *sane = sane_arg(argv[i]);

        char *slash = NULL;
        if (sane)
           slash = strchr(sane, '/');

        // if there's merge ongoing
        if (prefix) {
            // if arg is not sane (implied by !slash),
            // or if it is standalone (no slash)
            // or if prefix (incl. '/' after it) doesn't match
            if (!slash || (sane && (strncmp(sane, prefix, prefix_len) != 0 || sane[prefix_len] != '/'))) {
                // finish merge; process previously merged args
                if (execute(prefix, EXEC_TYPE_UNKNOWN, merged, basename, state) == -1)
                    goto err;
                free(prefix);
                prefix = NULL;
                merged_idx = 1;
                merged[merged_idx] = NULL;
            }
        }

        if (!sane) {
            ERROR_FMT("arg failed sanity check: %s\n", argv[i]);
            continue;
        }

        // standalone arg, just execute it, don't merge
        if (!slash) {
            if (execute(sane, EXEC_TYPE_UNKNOWN, merged, basename, state) == -1)
                goto err;
            continue;
        }

        // no merge in progress, start a new arglist
        if (!prefix) {
            prefix_len = slash-sane;
            prefix = strndup(sane, prefix_len);
            if (prefix == NULL) {
                PERROR("strndup");
                goto err;
            }
        }

        // appending to existing merge arglist
        merged[merged_idx] = sane+prefix_len+1;  // skip prefix + '/'
        merged[++merged_idx] = NULL;
    }

    // finish any merge-in-progress, execute it
    if (prefix)
        if (execute(prefix, EXEC_TYPE_UNKNOWN, merged, basename, state) == -1)
            goto err;

    free(prefix);
    free(merged);
    return destroy_exec_state(state);

err:
    free(prefix);
    free(merged);
    destroy_exec_state(state);
    return -1;
}
