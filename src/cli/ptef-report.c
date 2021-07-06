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

static void print_help(void)
{
    fprintf(stderr,
            "usage: ptef-report STATUS TEST\n"
            "\n"
            "Reports STATUS for a test named TEST, prepending PTEF_PREFIX\n"
            "to the TEST name, copying the report to PTEF_RESULTS_FD if defined.\n"
            "Outputs color if stdout is connected to a terminal.\n");
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        print_help();
        return 1;
    }

    return !!ptef_report(argv[1], argv[2]);
}
