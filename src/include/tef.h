#include <stdbool.h>

struct tef_runner_opts {
    char *argv0;
    unsigned int jobs;
    char **ignore_files;
    bool nomerge_args;
};

extern bool tef_runner(int argc, char **argv, struct tef_runner_opts *opts);