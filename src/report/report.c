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
# log a status for a test name (path)
_tef_log_status() {
    local status="$1" name="$2"

    if [ -t 0 ]; then
        case "$status" in
            PASS) echo -ne "\033[1;32m${status}\033[0m" >&0 ;;
            FAIL) echo -ne "\033[1;31m${status}\033[0m" >&0 ;;
            RUN) echo -ne "\033[1;34m${status} \033[0m" >&0 ;;
            MARK) echo -ne "\033[1;90m${status}\033[0m" >&0 ;;
            *) echo -n "$status" >&0 ;;
        esac
        echo " $TEF_PREFIX/$name" >&0
    else
        echo "$status $TEF_PREFIX/$name" 2>/dev/null >&0
    fi

    if [ -n "$TEF_RESULTS_FD" ]; then
        echo "$status $TEF_PREFIX/$name" 2>/dev/null >&"$TEF_RESULTS_FD"
    fi

    return 0
}
#endif


