#include <stdbool.h>

struct tef_runner_opts {
    char *argv0;
    int jobs;
    char **ignore_files;
    int nomerge_args;
};

extern bool tef_runner(int argc, char **argv, struct tef_runner_opts *opts);
extern bool tef_report(char *status, char *name);
