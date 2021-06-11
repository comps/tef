#
# Copyright (C) 2018  Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

#
## PRIVATE
#

_tef_normalize_path() {
    local path="$1"
    if [ "${path::1}" = "/" ]; then
        path=$(realpath -ms "$path")  # remove trailing '/', '//', '..', etc.
        echo "${path:1}"  # omit leading '/'
    else
        path=$(realpath -ms --relative-to="." "$path")
        echo "${path##*../}"  # trim possible leading '../../../'
    fi
}

# log a status for a test name (path)
_tef_log_status() {
    local status="$1" name="$2"

    if [ -t 1 ]; then
        case "$status" in
            PASS) echo -ne "\033[1;32m${status}\033[0m" ;;
            FAIL) echo -ne "\033[1;31m${status}\033[0m" ;;
            RUN) echo -ne "\033[1;34m${status} \033[0m" ;;
            MARK) echo -ne "\033[1;90m${status}\033[0m" ;;
            *) echo -n "$status" ;;
        esac
        echo " $TEF_PREFIX/$name"
    else
        echo "$status $TEF_PREFIX/$name" 2>/dev/null
    fi

    if [ -n "$TEF_RESULTS_FD" ]; then
        echo "$status $TEF_PREFIX/$name" 2>/dev/null >&"$TEF_RESULTS_FD"
    fi

    return 0
}

_tef_rotate_logfile() {
    local path="$1" i=
    [ -e "$path" ] || return 1
    for (( i = 7; i > 0; i-- )); do
        if [ -e "${path}.${i}" ]; then
            mv -f "${path}.${i}" "${path%%.$i}.$((i+1))"
        fi
    done
    rm -f "${path}.8"
    mv -f "$path" "${path}.1"
}

_tef_create_logroot() {
    if [ -n "$TEF_LOGS" ]; then
        [ -d "$TEF_LOGS" ] || mkdir "$TEF_LOGS" || return 1
        [ -d "$TEF_LOGS/$TEF_PREFIX" ] || \
            mkdir -p "$TEF_LOGS/$TEF_PREFIX" || return 1
        # no trailing slash on empty TEF_PREFIX
        echo "${TEF_LOGS}${TEF_PREFIX:+/$TEF_PREFIX}"
    else
        [ -d logs ] || mkdir logs || return 1
        echo logs
    fi
}

_tef_create_logfile() {
    local name="$1" path=

    path=$(_tef_create_logroot) || return 1
    path+="/${name}.log"

    _tef_rotate_logfile "$path"
    : > "$path" || return 1   # built-in touch
    echo "$path"
}

# if $1 is dir, run runner inside it, pass it args
# if $1 is exec file, run it, pass it args
_tef_run_child() {
    local args=("$@")
    args[0]="${args[0]#./}" # tidy up
    local child="${args[0]}"
    local logfile= rc= newlogs=

    if [ -d "$child" ]; then
        if [ -x "$child/$_tef_argv0_name" ]; then
            # if TEF_LOGS is relative, calculate new path to it from child
            if [ -n "$TEF_LOGS" -a "${TEF_LOGS::1}" != / ]; then
                newlogs=$(realpath --relative-to="$child" "$TEF_LOGS")
            fi
            logfile=$(_tef_create_logfile "$child") || return 1
            args[0]="./$_tef_argv0_name"
            _tef_log_status RUN "$child"
            {
                pushd "$child" >/dev/null
                if [ -n "$newlogs" ]; then
                    TEF_PREFIX="$TEF_PREFIX/$child" TEF_LOGS="$newlogs" "${args[@]}"
                else
                    TEF_PREFIX="$TEF_PREFIX/$child" "${args[@]}"
                fi
                rc=$?
                popd >/dev/null
            } 2> "$logfile"
            [ $rc -eq 0 ] && _tef_log_status PASS "$child" || _tef_log_status FAIL "$child"
        fi
    elif [ -x "$child" ]; then
        logfile=$(_tef_create_logfile "$child") || return 1
        _tef_log_status RUN "$child"
        args[0]="./$1"
        TEF_PREFIX="$TEF_PREFIX/$child" "${args[@]}" 2> "$logfile"
        [ $? -eq 0 ] && _tef_log_status PASS "$child" || _tef_log_status FAIL "$child"
    fi
}

_tef_run() {
    local child=

    # no args, run all without args
    if [ $# -eq 0 ]; then
        # do two loops to avoid stdin (fd 0) redirection issues;
        # (the < <(find..) would overwrite the original tty on fd 0)
        local children=()
        while read -r -d '' child; do
            children+=("$child")
        done < <(find . -mindepth 1 -maxdepth 1 \
                        -not -name "$_tef_argv0_name" -and -not -name '.*' \
                        -print0 | sort -z)
        for child in "${children[@]}"; do
            _tef_run_child "$child"
        done

    # some args - run only the specified tests
    else
        local argv=() arg= i=0 arr= prefix= postfix=

        for arg in "$@"; do
            arg=$(_tef_normalize_path "$arg")
            argv+=("$arg")
        done

        while [ "$i" -lt "${#argv[@]}" ]; do
            arr=()

            # gather all args with a common prefix in sequential order,
            # store their postfixes (arg without prefix)

            prefix="${argv[$i]%%/*}"
            postfix="${argv[$i]#*/}"
            [ "$postfix" = "${argv[$i]}" ] && postfix=

            # leaf part hit, no coalescing possible, just run
            if [ -z "$postfix" ]; then
                if [ -d "$prefix" -o -x "$prefix" ]; then
                    _tef_run_child "$prefix"
                else
                    echo "$prefix not dir/exec, skipping" >&2
                fi
                (( i++ ))
                continue
            fi

            # got a valid prefix and postfix
            for (( ; i < "${#argv[@]}" ; i++ )); do
                postfix="${argv[$i]#*/}"
                [ "$postfix" = "${argv[$i]}" ] && postfix=

                # if we hit a non-coalesce-able member, stop
                if [ -z "$postfix" -o "${argv[$i]%%/*}" != "$prefix" ]; then
                    break
                fi
                arr+=("$postfix")
            done

            # skip non-dirs and non-executables
            if [ -d "$prefix" -o -x "$prefix" ]; then
                arr=("$prefix" "${arr[@]}")
                _tef_run_child "${arr[@]}"
            else
                echo "$prefix not dir/exec, skipping" >&2
            fi
        done
    fi
    return 0
}

#
## PUBLIC
#

_tef_runner_help() {
    echo "Usage: tef-runner [-a argv0] [ARG]..."
    echo
    echo "-a    Override detected argv0 basename (for subdir execution)"
}

tef_runner() {
    local opt= OPTIND= OPTARG= OPTERR=

    while getopts ":a:h" opt; do
        case "$opt" in
            a)
                _tef_argv0_name="$OPTARG"
                ;;
            h)
                _tef_runner_help
                return 0
                ;;
            :)
                echo "missing argument for: -${OPTARG}" >&2
                _tef_runner_help >&2
                return 1
                ;;
            \?)
                echo "unknown argument: -${OPTARG}" >&2
                _tef_runner_help >&2
                return 1
                ;;
        esac
    done
    shift "$((OPTIND-1))"

    if [ -z "$_tef_argv0_name" ]; then
        {
            echo "the _tef_argv0_name variable is empty"
            echo "when running via a bash function, it must be defined to the"
            echo "basename of \$0 of the calling program, or set via the -a"
            echo "option passed to the tef_runner function"
        } >&2
        return 1
    fi

    _tef_run "$@"
}

_tef_report_help() {
    echo "Usage: tef-report <STATUS> <TESTNAME>"
}

tef_report() {
    local opt= OPTIND= OPTARG= OPTERR=

    while getopts ":h" opt; do
        case "$opt" in
            h)
                _tef_report_help
                return 0
                ;;
            :)
                echo "missing argument for: -${OPTARG}" >&2
                _tef_report_help >&2
                return 1
                ;;
            \?)
                echo "unknown argument: -${OPTARG}" >&2
                _tef_report_help >&2
                return 1
                ;;
        esac
    done
    shift "$((OPTIND-1))"

    if [ $# -ne 2 ]; then
        echo "incorrect arguments" >&2
        _tef_report_help >&2
        return 1
    fi

    _tef_log_status "$1" "$2"
}

_tef_mklog_help() {
    echo "Usage: tef-mklog [-d] <TESTNAME>"
    echo
    echo "-d    Create and printout logs dir, do not create test log"
}

tef_mklog() {
    local opt= OPTIND= OPTARG= OPTERR=

    while getopts ":dh" opt; do
        case "$opt" in
            d)
                _tef_create_logroot
                return 0
                ;;
            h)
                _tef_mklog_help
                return 0
                ;;
            :)
                echo "missing argument for: -${OPTARG}" >&2
                _tef_mklog_help >&2
                return 1
                ;;
            \?)
                echo "unknown argument: -${OPTARG}" >&2
                _tef_mklog_help >&2
                return 1
                ;;
        esac
    done
    shift "$((OPTIND-1))"

    if [ $# -ne 1 ]; then
        echo "incorrect arguments" >&2
        _tef_mklog_help >&2
        return 1
    fi

    _tef_create_logfile "$1"
}

# vim: syntax=sh sts=4 sw=4 et :
