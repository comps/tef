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

extern int fstatat_type(int dirfd, char *pathname, enum exec_entry_type *type);

// used across repeated execute() calls to track state
struct exec_state {
    bool failed;
    unsigned int running_jobs;
};

extern void
execute(char *exe, enum exec_entry_type typehint, char **argv,
        char *basename, struct exec_state *state);
