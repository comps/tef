#include <unistd.h>
#include <bash/builtins.h>
#include <bash/variables.h>
#include <bash/arrayfunc.h>
#include <bash/externs.h>
#include <bash/version.h>
#include <bash/config.h>

#include <ptef.h>

extern void builtin_error();

//
// runner
//

// bash goes into great lengts to override the glibc getenv/setenv functions
// for use with its own variables rather than the true environment ones
//
// this is useful for ie. report/mklog, because you can prefix them with
// variables on the CLI as if they were real commands, but breaks the runner
// when it tries to setenv() new PTEF_PREFIX and PTEF_LOGS in a fork()ed child
// just before execv(), because the setenv() doesn't actually set real env vars
// and thus the child execv()s with the old env var values
//
// therefore don't do runner as a bash builtin

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
