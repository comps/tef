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

#include <tef.h>

static void print_help(void)
{
    fprintf(stderr,
            "bla bla\n");
}

int main(int argc, char **argv)
{
#if 0
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
#endif
    if (argc < 3) {
        print_help();
        return 1;
    }

    return !!tef_report(argv[1], argv[2]);
}
