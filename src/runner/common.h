#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include <ptef.h>

// for passing file type between the directory scanner and executing
// function when running without args (when scanning a directory),
// to prevent an extra fstatat()
// - when running with args (no directory is scanned), UNKNOWN is used
//   and the executing function will do the fstatat() itself
enum exec_entry_type {
    EXEC_TYPE_UNKNOWN,
    EXEC_TYPE_INVALID,
    EXEC_TYPE_FILE,
    EXEC_TYPE_DIR,
};

int fstatat_type(int dirfd, char *pathname, enum exec_entry_type *type);

// for parallel execution - we need to keep track of persistent
// pid-to-testname mapping, so we can ptef_report the correct name
// when a child finishes (might be many iterations later)
struct pid_to_name {
    pid_t pid;
    char *name;
};
// used across repeated execute() calls to track state
struct exec_state {
    int max_jobs;
    int running_jobs;
    struct pid_to_name map[];
};

struct exec_state *create_exec_state(struct ptef_runner_opts *opts);
int destroy_exec_state(struct exec_state *state);
int execute(char *exe, enum exec_entry_type typehint, char **argv,
            struct ptef_runner_opts *opts, struct exec_state *state);

int for_each_arg(int argc, char **argv, struct ptef_runner_opts *opts);

int for_each_merged_arg(int argc, char **argv, struct ptef_runner_opts *opts);

int for_each_exec(struct ptef_runner_opts *opts);
