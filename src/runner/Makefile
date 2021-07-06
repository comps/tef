.PHONY: all
all: runner.o common.o execute.o with_args.o without_args.o

CFLAGS ?= -O2 -flto -Wall -g
CFLAGS += -I../include -I../helpers -fPIC

runner.o: runner.c
common.o: common.c
execute.o: execute.c
with_args.o: with_args.c
without_args.o: without_args.c

.PHONY: clean
clean:
	rm -f runner.o common.o execute.o with_args.o without_args.o
