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

#include <tef.h>
#include "execute.h"

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
bool
for_each_arg(int argc, char **argv, struct tef_runner_opts *opts)
{
    struct exec_state state = { 0 };
    // TODO: simply iterate over argv, skip (continue) over !sane_arg(),
    //       call execute(), nothing fancy
    return state.failed;
}
bool
for_each_merged_arg(int argc, char **argv, struct tef_runner_opts *opts)
{
    char **merged;
    struct exec_state state = { 0 };

    // +2 is for argv[0] and terminating NULL ptr
    if ((merged = malloc((argc+2)*sizeof(argv))) == NULL) {
        perror("malloc");
        return false;
    }
    // leave 0 for execve argv[0] being executable name
    int merged_idx = 1;
    // NULL-terminate, will be moved on each arg addition
    merged[merged_idx] = NULL;

    char *prefix = NULL;
    ptrdiff_t prefix_len = 0;

    for (int i = 0; i < argc; i++) {
        char *sane = sane_arg(argv[i]);
        if (!sane)
            continue;

        char *slash = strchr(sane, '/');

        // if there's merge ongoing
        if (prefix) {
            // if standalone arg is found or if prefix (incl. '/' after it) doesn't match
            if (!slash || strncmp(sane, prefix, prefix_len) != 0 || sane[prefix_len] != '/') {
                // finish merge; process previously merged args
                execute(prefix, EXEC_TYPE_UNKNOWN, merged, opts->argv0, &state);
                free(prefix);
                prefix = NULL;
                merged_idx = 1;
                merged[merged_idx] = NULL;
            }
        }

        // standalone arg, just execute it, don't merge
        if (!slash) {
            execute(sane, EXEC_TYPE_UNKNOWN, NULL, opts->argv0, &state);
            continue;
        }

        // no merge in progress, start a new arglist
        if (!prefix) {
            prefix_len = slash-sane;
            prefix = strndup(sane, prefix_len);
            if (prefix == NULL)
                goto err;
        }

        // appending to existing merge arglist
        merged[merged_idx] = sane+prefix_len+1;  // skip prefix + '/'
        merged[++merged_idx] = NULL;
    }

    // finish any merge-in-progress, execute it
    if (prefix)
        execute(prefix, EXEC_TYPE_UNKNOWN, merged, opts->argv0, &state);

    return state.failed;

err:
    free(prefix);
    free(merged);
    return false;
}
