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

#if 0
static void print_help(void)
{
    fprintf(stderr,
            "bla bla\n");
}

int tef_runner(int argc, char **argv, unsigned int flags, char **ignore_files)
{
    if (argc < 2)
        return 1;

    while ((int c = getopt(argc, argv, "+:f:t:s:n:b:h")) != -1) {
        switch (c) {
            case 'f':
                /* source offset */
                offset_from = strtoullx(optarg);
                args_read |= (1<<0);
                break;
            case 't':
                /* destination offset */
                offset_to = strtoullx(optarg);
                args_read |= (1<<1);
                break;
            case 'b':
                /* block size */
                block_size = strtoullx(optarg);
                if (!errno && block_size < 1) {
                    fprintf(stderr, "Block size needs to be >= 1\n");
                    return 1;
                }
                break;
            case 'h':
                print_help();
                return 0;
            case ':':
                // TODO missing argument to opt
                return 1;
            case '?':
                // TODO other error
                return 1;
        }

        /* one of the strtoullx calls failed */
        if (errno)
            exit(EXIT_FAILURE);
    }

    //for_each_exec(argv[1]);
    for_each_arg(basename(argv[0]), argc-1, argv+1);
    return 0;
}
#endif

#include <tef.h>

int main(int argc, char **argv)
{
    struct tef_runner_opts opts = { 0 };

    opts.argv0 = basename(argv[0]);
    //char ign[2][20] = { "zanother", NULL };
    char *ign[2] = { 0 };
    //ign[0] = "zanother";
    ign[1] = NULL;
    opts.ignore_files = ign;

    return !tef_runner(argc-1, argv+1, &opts);
}
