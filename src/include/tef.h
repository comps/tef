#ifndef _TEF_H
#define _TEF_H

struct tef_runner_opts {
    char *argv0;
    int jobs;
    char **ignore_files;
    int nomerge_args;
};

extern int tef_runner(int argc, char **argv, struct tef_runner_opts *opts);
extern int tef_report(char *status, char *name);

#endif
