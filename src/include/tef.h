#ifndef _TEF_H
#define _TEF_H

// options passed to tef_runner to parametrize a run,
// zero out the whole struct for defaults
struct tef_runner_opts {
    // argv0 of the runner, used for searching subfolders and recursion
    // if NULL, the runner will never recurse
    char *argv0;  // TODO: set "" default in tef_runner
    // nr. of tests to run in parallel, 0 or 1 for sequential
    int jobs;
    // file names to ignore when scanning for executables
    // during an argument-less run
    char **ignore_files;
    // don't try to merge arguments during an argument-based run,
    // run each executable with at most 1 argument
    int nomerge_args;
    // don't redirect stderr of tests to a log, leave it on 2
    // useful for debugging
    int verbose;  // TODO: don't redirect stderr
};

int tef_runner(int argc, char **argv, struct tef_runner_opts *opts);
int tef_report(char *status, char *name);
int tef_mklog(char *testname);

#endif
