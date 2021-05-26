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

#include "common.h"

// free the block if realloc fails
void *realloc_safe(void *ptr, size_t size)
{
    void *new = realloc(ptr, size);
    if (new == NULL)
        free(ptr);
    return new;
}

int fstatat_type(int dirfd, char *pathname, enum exec_entry_type *type)
{
    int ret;
    struct stat statbuf;
    if ((ret = fstatat(dirfd, pathname, &statbuf, 0)) == -1)
        return -1;
    switch (statbuf.st_mode & S_IFMT) {
        case S_IFREG:
            *type = EXEC_TYPE_FILE;
            break;
        case S_IFDIR:
            *type = EXEC_TYPE_DIR;
            break;
        default:
            *type = EXEC_TYPE_INVALID;
            break;
    }
    return ret;
}
