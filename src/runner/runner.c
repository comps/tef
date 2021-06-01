#include <stdbool.h>

//#define _POSIX_C_SOURCE 200809L

#include "common.h"

// true if everything PASSed, false on error or FAIL
__asm__(".symver tef_runner_v0, tef_runner@@VERS_0");
bool tef_runner_v0(int argc, char **argv, struct tef_runner_opts *opts)
{
    // sanitize opts
    if (opts->jobs < 1)
        opts->jobs = 1;

    if (argc <= 0)
        return for_each_exec(opts);
    if (opts->nomerge_args)
        return for_each_arg(argc, argv, opts);
    else
        return for_each_merged_arg(argc, argv, opts);
}
