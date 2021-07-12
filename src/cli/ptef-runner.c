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

#include <ptef.h>
#include <ptef_helpers.h>

static void print_help(void)
{
    fprintf(stderr,
            "usage: ptef-runner [OPTIONS] [TEST]...\n"
            "\n"
            "  -a BASE  argv0 basename, overriding autodetection\n"
            "  -j NR    number of parallel jobs (tests)\n"
            "  -i IGN   set PTEF_IGNORE_FILES to IGN, export it\n"
            "  -n       don't merge arguments of subrunners (always pass 1 arg)\n"
            "\n"
            "Executes the PTEF runner logic from CWD, executing executables\n"
            "and traversing subdirectories.\n"
            "If TEST is specified, runs only that test, without scanning for\n"
            "executables.\n");
}

int main(int argc, char **argv)
{
    struct ptef_runner_opts opts = { 0 };

    int c;
    while ((c = getopt(argc, argv, "a:j:i:nh")) != -1) {
        switch (c) {
            case 'a':
                opts.argv0 = optarg;
                break;
            case 'j':
                opts.jobs = atoi(optarg);
                if (opts.jobs < 1) {
                    ERROR_FMT("invalid job cnt: %s\n", optarg);
                    return 1;
                }
                break;
            case 'i':
                if (setenv("PTEF_IGNORE_FILES", optarg, 1) == -1) {
                    PERROR("setenv");
                    return 1;
                }
                break;
            case 'n':
                opts.nomerge_args = 1;
                break;
            case 'h':
                print_help();
                return 0;
            case '?':
                return 1;
        }
    }

    if (!opts.argv0)
        opts.argv0 = basename(argv[0]);

    switch (ptef_runner(argc-optind, argv+optind, &opts)) {
        // success
        case 0:
            return 0;
        // internal error
        case -1:
            return 1;
        // test error
        default:
            return 2;
    }
}
