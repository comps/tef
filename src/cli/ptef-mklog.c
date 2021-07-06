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
            "usage: ptef-mklog TEST\n"
            "\n"
            "Creates and opens a log storage for a test named TEST, relaying\n"
            "stdin to the log storage until EOF is encountered.\n");
}

#define BUF_SIZE 1024
static int relay_input(int to)
{
    char *buf;
    if ((buf = malloc(BUF_SIZE)) == NULL) {
        PERROR("malloc");
        return -1;
    }
    ssize_t bytes;
    while ((bytes = read(STDIN_FILENO, buf, BUF_SIZE)) > 0) {
        if (write_safe(to, buf, bytes) == -1) {
            free(buf);
            return -1;
        }
    }
    free(buf);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_help();
        return 1;
    }

    int fd;
    if ((fd = ptef_mklog(argv[1])) == -1)
        return 1;

    if (relay_input(fd) == -1) {
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
