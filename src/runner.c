//#define _POSIX_C_SOURCE 200809L

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

#include "tef.h"

// free the block if realloc fails
static void *realloc_safe(void *ptr, size_t size)
{
    void *new = realloc(ptr, size);
    if (new == NULL)
        free(ptr);
    return new;
}

enum exec_entry_type {
    EXEC_TYPE_UNKNOWN,
    EXEC_TYPE_INVALID,
    EXEC_TYPE_FILE,
    EXEC_TYPE_DIR,
};
struct exec_entry {
    enum exec_entry_type type;
    char name[256];  // same as struct dirent
};
static int exec_entry_sort_cmp(const void *a, const void *b)
{
    const struct exec_entry *enta = a, *entb = b;
    return strcoll((const char *)enta->name, (const char *)entb->name);
}
static int
fstatat_type(int dirfd, char *pathname, enum exec_entry_type *type)
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

// used across repeated execute() calls to track state
struct exec_state {
    bool failed;
    unsigned int running_jobs;
};

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
static void
execute(char *exe, enum exec_entry_type typehint, char **argv,
        char *basename, struct exec_state *state)
{
    if (typehint == EXEC_TYPE_UNKNOWN) {
        if (fstatat_type(AT_FDCWD, exe, &typehint) == -1) {
            perror("execute fstatat");
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

//
// argument-less run
//

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
            perror("faccessat");
        return false;
    }
    return true;
}
static int
find_execs(struct exec_entry ***entries, char *basename, char **ignored)
{
    DIR *cwd = NULL;
    if ((cwd = opendir(".")) == NULL) {
        perror("opendir");
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
                    perror("openat");
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
            perror("realloc");
            goto err;
        }
        entcnt++;

        struct exec_entry *ent;
        if ((ent = malloc(sizeof(*ent))) == NULL) {
            perror("malloc");
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
static bool for_each_exec(struct tef_runner_opts *opts)
{
    struct exec_entry **ents;
    int cnt;
    struct exec_state state = { 0 };

    cnt = find_execs(&ents, opts->argv0, opts->ignore_files);
    if (cnt == -1)
        return false;

    for (int i = 0; i < cnt; i++) {
        execute(ents[i]->name, ents[i]->type, NULL, opts->argv0, &state);
        free(ents[i]);
    }
    free(ents);

    return state.failed;
}

//
// argument-given run
//

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
static bool
for_each_arg(int argc, char **argv, struct tef_runner_opts *opts)
{
    struct exec_state state = { 0 };
    // TODO: simply iterate over argv, skip (continue) over !sane_arg(),
    //       call execute(), nothing fancy
    return state.failed;
}
static bool
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

bool tef_runner(int argc, char **argv, struct tef_runner_opts *opts)
{
    if (argc <= 0)
        return for_each_exec(opts);
    if (opts->nomerge_args)
        return for_each_arg(argc, argv, opts);
    else
        return for_each_merged_arg(argc, argv, opts);
}


int main(int argc, char**argv)
{
    struct tef_runner_opts opts = { 0 };

    opts.argv0 = basename(argv[0]);
    //char ign[2][20] = { "zanother", NULL };
    char *ign[2] = { 0 };
    //ign[0] = "zanother";
    ign[1] = NULL;
    opts.ignore_files = ign;

    return !tef_runner(argc-1, argv+1, &opts);
}
