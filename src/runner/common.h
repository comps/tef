#include <stdbool.h>
#include <stddef.h>

#include <tef.h>

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

int fstatat_type(int dirfd, char *pathname, enum exec_entry_type *type);

// used across repeated execute() calls to track state
struct exec_state {
    bool failed;
    int running_jobs;
};

int execute(char *exe, enum exec_entry_type typehint, char **argv,
            struct tef_runner_opts *opts, struct exec_state *state);

int for_each_arg(int argc, char **argv, struct tef_runner_opts *opts);

int for_each_merged_arg(int argc, char **argv, struct tef_runner_opts *opts);

int for_each_exec(struct tef_runner_opts *opts);
