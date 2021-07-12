//#define _POSIX_C_SOURCE 200809L

#include "common.h"

// true if everything PASSed, false on error or FAIL
__asm__(".symver ptef_runner_v0, ptef_runner@@VERS_0");
__attribute__((used))
int ptef_runner_v0(int argc, char **argv, char *basename, int jobs, int nomerge)
{
    // sanitize
    if (jobs < 1)
        jobs = 1;

    if (argc <= 0)
        return for_each_exec(basename, jobs);
    if (nomerge)
        return for_each_arg(argc, argv, basename, jobs);
    else
        return for_each_merged_arg(argc, argv, basename, jobs);
}
