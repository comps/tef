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
            "bla bla\n");
}

int main(int argc, char **argv)
{
    struct ptef_runner_opts opts = { 0 };

    int ignored_cnt = 1;  // terminating NULL

    int c;
    while ((c = getopt(argc, argv, "a:j:i:nh")) != -1) {
        switch (c) {
            case 'a':
                opts.argv0 = optarg;
                break;
            case 'j':
                opts.jobs = atoi(optarg);
                break;
            case 'i':
                ignored_cnt++;
                opts.ignore_files =
                    realloc_safe(opts.ignore_files, sizeof(char**)*ignored_cnt);
                opts.ignore_files[ignored_cnt-2] = optarg;
                opts.ignore_files[ignored_cnt-1] = NULL;
                break;
            case 'n':
                opts.nomerge_args = true;
                break;
            case 'h':
                print_help();
                return 0;
            case ':':
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
