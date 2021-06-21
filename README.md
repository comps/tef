# POSIX-inspired Test Execution Framework (PTEF)

A (fairly) simple specification and an example implementation of a test "runner"
framework for system testing and/or integration and execution of test suites.

Currently still in active development (unstable API).

## Basic intro

### What is PTEF?

A specification of API for test execution. It tells how to run tests/suites or
(in general) any executables, how to manage and recurse into subtrees of tests,
how to report results, where and how to store output from tests, etc.
All using standard POSIX interfaces.

All of this could easily be created from scratch for an individual project,
but PTEF aims to render this unnecessary and unify simple test execution.

### What isn't PTEF?

A test library. PTEF doesn't provide any helper functions for tests themselves,
only for management of their execution.

### Why should I want it?

* It takes care of the topmost system level of test execution - ideal when you
  have lots of tests and know how to write them, but don't have a good way
  to run them in an organized way

* It integrates well, both down and up. Downwards with any tests or test suites
  that are POSIX-compatible (most programming languages), ie. bash/python/C/etc.
  Upwards with any execution managers that have a POSIX interface (incl. any
  that can run a shell), ie. Jenkins, Travis CI, or a human being.

* It's just a specification. If you don't like the reference implementation,
  you can write you own in whatever language you like.

* In fact, it's encouraged to have multiple (even partial) implementations of
  the specification (or alteration of an existing one) for specific needs, such
  as execution of a monolithic 3rd party test suite.

* It's KISS. It's small and does what it was designed to do, nothing more.
  Any non-essential parts are either intentionally left undefined or optional.

### TODO

- install valgrind before `make test`
