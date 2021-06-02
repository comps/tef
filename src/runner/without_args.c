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
#include <tef_helpers.h>

#include "common.h"

static int exec_entry_sort_cmp(const void *a, const void *b)
{
    const struct exec_entry *enta = a, *entb = b;
    return strcoll((const char *)enta->name, (const char *)entb->name);
}

// like scandir, but customized for our use case, also without the need
// to pass argv[0] via a global var to a scandir (*filter)
// - also returns struct exec_entry, not just a name
//
// filter valid executables/dirs here, so that anything we pass to
// execute() can be treated as an error if execution fails
static bool is_exec(int parentfd, char *name)
{
    int ret;
    if ((ret = faccessat(parentfd, name, X_OK, 0)) == -1) {
        if (errno != EACCES && errno != ENOENT)
            PERROR_FMT("faccessat %s", name);
        return false;
    }
    return true;
}

static int
find_execs(struct exec_entry ***entries, char *basename, char **ignored)
{
    DIR *cwd = NULL;
    if ((cwd = opendir(".")) == NULL) {
        PERROR("opendir CWD");
        return -1;
    }

    int cwdfd = dirfd(cwd);
    struct exec_entry **ents = NULL;
    size_t entcnt = 0;

    struct dirent *dent;
    while ((dent = readdir(cwd)) != NULL) {
        // skip hidden files, '.' and '..'
        if (dent->d_name[0] == '.')
            continue;

        // skip current executable
        if (strcmp(dent->d_name, basename) == 0)
            continue;

        // skip user-ignored names
        if (ignored) {
            int i;
            for (i = 0; ignored[i] != NULL; i++) {
                if (strcmp(dent->d_name, ignored[i]) == 0)
                    break;
            }
            if (ignored[i] != NULL)
                continue;
        }

        enum exec_entry_type enttype;

#if defined(_DIRENT_HAVE_D_TYPE) && defined(DT_REG) && defined(DT_DIR)
        switch (dent->d_type) {
            case DT_REG:
                enttype = EXEC_TYPE_FILE;
                break;
            case DT_DIR:
                enttype = EXEC_TYPE_DIR;
                break;
            default:
                continue;
        }
#else
        if (fstatat_type(cwdfd, dent->d_name, &enttype) == -1)
            continue;
        if (enttype == EXEC_TYPE_INVALID)
            continue;
#endif

        int subdir;
        switch (enttype) {
            case EXEC_TYPE_DIR:
                // look for our basename in the directory
                if ((subdir = openat(cwdfd, dent->d_name, O_DIRECTORY)) == -1) {
                    PERROR_FMT("openat %s", dent->d_name);
                    continue;
                }
                if (!is_exec(subdir, basename)) {
                    close(subdir);
                    continue;
                }
                close(subdir);
                break;
            case EXEC_TYPE_FILE:
                // just check for executability
                if (!is_exec(cwdfd, dent->d_name))
                    continue;
                break;
            default:
                continue;
        }

        if ((ents = realloc_safe(ents, (entcnt+1)*sizeof(*ents))) == NULL) {
            PERROR("realloc");
            goto err;
        }
        entcnt++;

        struct exec_entry *ent;
        if ((ent = malloc(sizeof(*ent))) == NULL) {
            PERROR("malloc");
            goto err;
        }
        strncpy(ent->name, dent->d_name, sizeof(ent->name));
        ent->type = enttype;

        ents[entcnt-1] = ent;
    }

    qsort(ents, entcnt, sizeof(*ents), exec_entry_sort_cmp);
    closedir(cwd);
    *entries = ents;
    return entcnt;

err:
    while (entcnt--)
        free(ents[entcnt]);
    free(ents);
    closedir(cwd);
    return -1;
}

int for_each_exec(struct tef_runner_opts *opts)
{
    struct exec_entry **ents;
    int cnt;
    struct exec_state state = { 0 };

    cnt = find_execs(&ents, opts->argv0, opts->ignore_files);
    if (cnt == -1)
        return -1;

    char *argv[2] = { NULL };
    for (int i = 0; i < cnt; i++) {
        execute(ents[i]->name, ents[i]->type, argv, opts, &state);
        free(ents[i]);
    }
    free(ents);

    return state.failed;
}
