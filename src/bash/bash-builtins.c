#include <unistd.h>
#include <bash/builtins.h>
//#include <bash/builtins/common.h>
#include <bash/builtins/bashgetopt.h>
#include <bash/variables.h>
#include <bash/arrayfunc.h>
#include <bash/externs.h>
#include <bash/version.h>
#include <bash/config.h>

#include <ptef.h>

extern void builtin_error();
extern void builtin_usage();
extern char **make_builtin_argv();

//
// runner
//

// the libgen.h version may modify path
static char *basename(char *path)
{
    char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

static int runner_main(WORD_LIST *list)
{
    struct ptef_runner_opts opts = { 0 };

    reset_internal_getopt();
    int opt;
    while ((opt = internal_getopt(list, "a:j:i:nh")) != -1) {
        switch (opt) {
            case 'a':
                opts.argv0 = list_optarg;
                break;
            case 'j':
                opts.jobs = atoi(list_optarg);
                if (opts.jobs < 1) {
                    builtin_error("invalid job cnt: %s", list_optarg);
                    return 1;
                }
                break;
            case 'n':
                opts.nomerge_args = 1;
                break;
            case GETOPT_HELP:
                builtin_usage();
                return 0;
            default:
                builtin_usage();
                return 1;
        }
    }
    list = loptend;

    if (!opts.argv0)
        opts.argv0 = basename(dollar_vars[0]);

    int argc;
    char **argv = strvec_from_word_list(list, 0, 0, &argc);
    int ret = ptef_runner(argc, argv, &opts);
    free(argv);
    switch (ret) {
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

    return 0;
}

//
// report
//

static int report_main(WORD_LIST *status)
{
    if (!status) {
        builtin_error("not enough arguments");
        return 1;
    }
    WORD_LIST *test = status->next;
    if (!test) {
        builtin_error("not enough arguments");
        return 1;
    }
    if (test->next) {
        builtin_error("too many arguments");
        return 1;
    }
    return !!ptef_report(status->word->word, test->word->word);
}

//
// mklog
//

#if DEFAULT_COMPAT_LEVEL < 51
static SHELL_VAR *builtin_bind_variable(char *name, char *value, int flags)
{
    SHELL_VAR *v;
#ifdef ARRAY_VARS
    if (!valid_array_reference(name, 0))
        v = bind_variable(name, value, flags);
    else
        v = assign_array_element(name, value, flags);
#else
    v = bind_variable(name, value, flags);
#endif
    if (v && !readonly_p(v) && !noassign_p(v))
        VUNSETATTR(v, att_invisible);
    return v;
}
#endif

static int mklog_bind_variable(char *name, int value)
{
    SHELL_VAR *v;
    char buf[INT_STRLEN_BOUND(int)+1], *ptr;
    ptr = fmtulong(value, 10, buf, sizeof(buf), 0);
    v = builtin_bind_variable(name, ptr, 0);
    if (!v || readonly_p(v) || noassign_p(v))
        builtin_error("%s: cannot set variable", name);
    return !!v;
}

static int mklog_main(WORD_LIST *test)
{
    if (!test) {
        builtin_error("not enough arguments");
        return 1;
    }
    WORD_LIST *varname = test->next;
    if (!varname) {
        builtin_error("not enough arguments");
        return 1;
    }
    if (varname->next) {
        builtin_error("too many arguments");
        return 1;
    }
    int fd = ptef_mklog(test->word->word);
    if (fd == -1) {
        builtin_error("returned -1");
        return 1;
    }
    if (!mklog_bind_variable(varname->word->word, fd)) {
        close(fd);
        return 1;
    }
    return 0;
}

//
// builtin boilerplate
//

static char *runner_help[] = {
    "  -a BASE  argv0 basename, overriding autodetection",
    "  -j NR    number of parallel jobs (tests)",
    "  -i EXE   ignore exe filename, can specify multiple times",
    "  -n       don't merge arguments of subrunners (always pass 1 arg)",
    "",
    "Executes the PTEF runner logic from CWD, executing executables",
    "and traversing subdirectories.",
    "If TEST is specified, runs only that test, without scanning for",
    "executables.",
    NULL
};
static char *report_help[] = {
    "Reports STATUS for a test named TEST, prepending PTEF_PREFIX",
    "to the TEST name, copying the report to PTEF_RESULTS_FD if defined.",
    "Outputs color if stdout is connected to a terminal.",
    NULL
};
static char *mklog_help[] = {
    "Creates and opens a log storage for a test named TEST,",
    "binds its file descriptor number to a variable named VARNAME.",
    NULL
};
// must be named <name>_struct
// name, function, flags, log_doc, short_doc, handle (unused)
struct builtin ptef_runner_struct = {
    "ptef_runner",
    runner_main,
    BUILTIN_ENABLED,
    runner_help,
    "ptef_runner [OPTIONS] [TEST]...",
    NULL
};
struct builtin ptef_report_struct = {
    "ptef_report",
    report_main,
    BUILTIN_ENABLED,
    report_help,
    "ptef_report STATUS TEST",
    NULL
};
struct builtin ptef_mklog_struct = {
    "ptef_mklog",
    mklog_main,
    BUILTIN_ENABLED,
    mklog_help,
    "ptef_mklog TEST VARNAME",
    NULL
};
