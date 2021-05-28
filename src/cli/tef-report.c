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

//#include <tef.h>
//#include "../runner/common.h"

static void print_help(void)
{
    fprintf(stderr,
            "bla bla\n");
}

int main(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
            case 'h':
                print_help();
                return 0;
            case ':':
            case '?':
                return 1;
        }

        /* one of the strtoullx calls failed */
        //if (errno)
        //    exit(EXIT_FAILURE);
    }

    //if (!opts.argv0)
    //    opts.argv0 = basename(argv[0]);

    //printf("%s // %d : %d\n", argv[optind], optind, argc);
    //return !tef_runner(argc-optind, argv+optind, &opts);
    return 0;
}
