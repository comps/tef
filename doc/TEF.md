# Test Execution Framework (TEF) - Specification

Version 0.7

// TODO: TEF_INTERACTIVE support
//  - when present in env and non-empty and if no args are given to the runner,
//    launch interactive $SHELL instead of running all tests
//    - ie. 'runner /a/b/c' would execute 'c/runner' without args, which would
//      spawn $SHELL
//  - very useful for debugging when there's some state in setup/cleanup
//    provided by parent runners

// TODO: TEF_DEBUG_OUTPUT (?) support
//  - when present in env and non-empty, do not redirect stdout/err of execs
//  - this means all output of all runners / tests (all executables) will
//    interleave with fd0 reported statuses
//  - again, very useful for debug runs of just a few tests + collecting all
//    logs on the same console stream without having to read logs dir

## Unspecified/undefined behavior

Anything outside this specification is implementation-dependent.

This document declares the minimum mandatory interface, other documents then
supplement it with recommendations and rationale.

## Hierarchy

The suite consists of hiearchical "levels", separated by directory levels
(or virtual/imaginary levels), with each level having one "runner" responsible
for test execution and results reporting. The API between levels is UNIX/POSIX,
so the runners can be implemented in different programming languages and don't
rely on implementation specifics.

Symbolic links (files) are always dereferenced when accessing a directory or
executing a file.

The primary function of the test "runner" is to iterate over the contents of
the current working directory (CWD):

* If a directory is encountered, it makes an attempt to find an executable file
  matching the current basename of `argv[0]` in this directory and execute it
  with CWD of that directory

* If an executable file is encountered and it doesn't match the basename of
  `argv[0]`, it is executed

"Basename" being the last member of the path, usually a filename.

## Argument passing

When called without CLI arguments, the behavior of the runner is undefined.

// ^^^ revisit that, define it as 'runs maximum possible test set' or so,
// the corner cases of ie. 'setup' vs 'run' executables and one running
// the other because != basename(argv[0]) are .. corner cases, easy deviations
// from the spec

When called with arguments, the runner must execute exactly the files or
directories specified by the arguments.

An argument specifies the executable file names or relative paths leading into
subdirectories. For example `a` or `a/b/c`.

Before executing a file or a runner inside a directory, the topmost level
of the path in the argument is stripped and the result is passed to the
executable/runner.

For example:
```
/a/b/c/d/e

- runner executable 'builder' in 'a' runs with argument 'b/c/d'
- it finds that 'b' is a directory
- it looks for an executable in 'b' called 'builder'
- it runs the executable, passing 'c/d' to it
```

Note that argument stripping is not tied to the nature of the executed file,
it is done both for a runner in a subdirectory as well as an executable in the
current directory, ie.
```
/a/b

- runner executable 'builder' in 'a' runs with argument 'b/c/d'
- it finds that 'b' is an executable file
- it runs 'b', passing 'c/d' as an argument to it
```

// any leading slashes should be removed (if they exist)
// the first path element (until / or end of string) should be examined:
// - if it has zero length,
// - if it contains '.' or '..',
// it is invalid and the entire argument must be skipped, error optionally logged
// -- TODO in Optional.md - merging must stop on invalid cli argument
//
// the runner must not parse the path beyond the first element
// (it must stop at the first '/' or end of string)


## Exit codes

If an executable fails (returns a non-0 exit code), the current "level" of
the hierarchy is considered as failed and the runner must exit with a non-0
exit code once all executables finish.

This means that a failure anywhere will propagate upwards the hierarchy.

Ie.
```
/a/b

- runner executable 'builder' in 'b' runs executables 'c1', 'c2', 'c3'
- 'c1' returns 0, 'c2' returns 2, 'c3' returns 0
- 'builder' in 'b' returns 1
- its parent in 'a' returns 1
```

## Result reporting

Result reporting is done by every runner (every recursion level).

If the runner process has stdin (fd 0) open for writing, such as when
it is connected to a tty, it outputs status reporting lines on it with
the following format:
```
STATUS executable_name
```
where STATUS is one of PASS (retval 0) or FAIL (retval non-0) for a finished
execution or RUN for an executable that is being started, meaning the log could
look like ie.
```
RUN first_executable
PASS first_executable
RUN second_executable
FAIL second_executable
```
The lines may be interleaved if the executables run in parallel. In that case,
the runner needs to lock fd 0 to get exclusive access for writing. This is done
using POSIX advisory record locks (the `fcntl` variant) with maximum byte range
specified (`l_type=F_WRLCK`,`l_whence=SEEK_SET`,`l_start=0`,`l_len=0`).

Since the runner has no notion of its location within the hierarchy, it needs
to prefix each executable name with the contents of `TEF_PREFIX` env variable,
followed by a forward slash. If the variable doesn't exist, it is defined as
an empty string.

When executing a runner inside a subdirectory, the subdirectory name is appended
to the `TEF_PREFIX` variable passed to the runner, separated by a forward slash.
When executing a regular executable, the name of the executable is appended
instead.

Thus, the actual results could look like ie.
```
RUN /first_executable
PASS /first_executable
RUN /subdir/second_executable
FAIL /subdir/second_executable
```
The first two lines being from a first runner, the last two lines from another
runner inside a `subdir` directory, to which the first runner passed
`TEF_PREFIX` equal to `/subdir`.

The status may be colorized or bold/underline/etc., but only if the output fd is
a terminal (`tcgetattr(3)`). Terminfo should be queried for safety.

There may be any number of spaces/tabs between the status and the test name,
however at least one is mandatory.

The use of the RUN status is recommended and default, but it may be omitted
where appropriate.

### Timestamps and the MARK status

A runner may, at any time, output a result with a MARK status with the current
wall clock time in place of the executable name. The time must be in an ISO 8601
format and may optionally include a timezone and/or nanosecond information.

Ie.
```
MARK 2021-04-07T08:15:46+02:00
MARK 2021-04-07T08:20:36,909601272+02:00
```

The runner may also output this result while an executable is running (ie. from
a separate thread), assuming it acquired an appropriate lock.

### Result reporting for logging

If the `TEF_RESULTS_FD` environment variable is set, the same output that is
sent to fd 0 (see above) is also copied to the file descriptor specified by the
variable.

Any special characters (color, formatting) is omitted, the output on this fd
must match the original data (exact status and exec path).

Locking via POSIX advisory locks (see above) applies too. The order of entries
may however be different, holding exclusive access to both fd 0 and
`TEF_RESULTS_FD` is not required.

### Usage of stdout and stderr

The runner shall use output stdio for any relevant debugging, informatory or
error output. The format is undefined here.

During normal and default operation, no output should be written to stdout or
stderr.

## Logging

When executing any executable, the runner redirects both stdout and stderr
to a log file.

### Logging in a separate hierarchy

If the `TEF_LOGS` environment variable is non-empty, it specifies the root
of a separate logging hierarchy, which mimics the execution hierarchy.

The `TEF_LOGS` directory and any sub-directories inside it are created
if non-existent.

In this hierarchy, the log files match the name of the executables. When
executing a directory (executable with `argv[0]` name in a directory), the name
of the directory is used instead.

Since directories and log files from `argv[0]` runners of the directories cannot
share the same name, `.log` is appended to all log files inside the hierarchy.

Ie. under `TEF_LOGS`:
```
/first_executable.log
/subdir/second_executable.log
/subdir.log
```

The log files are always opened for overwriting (without `O_APPEND`).

### Logging in CWD

If the `TEF_LOGS` environment variable is nonexistent or empty, a `logs`
directory (created if non-existent) in the current working directory is used
instead.

The `logs` directory contains only logs for executables on the current level,
eg. it never contains any subdirectories.

For consistency, `.log` is appended to all log files, as before.

If it is logically impossible to store logs (due to virtual hierarchies which
cannot hold real directories with logs), no IO redirection is performed. The
parent runner thus collects the outputs in the current runner's log file.

Ie. under CWD:
```
logs/first_executable.log
logs/subdir.log
```
and under `subdir` in CWD:
```
logs/second_executable.log
```

## Additional functionality

See other related documents that expand on this specification to provide
a more user-friendly interface and richer feature set.

## Examples

See other documents for further examples explaining these basic concepts.

Also see the `examples` directory for practical deployment demonstration.
