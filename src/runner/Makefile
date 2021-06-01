.PHONY: all
all: runner.o common.o execute.o with_args.o without_args.o

CFLAGS += -I../include -fPIC -Wall -g

runner.o: runner.c
common.o: common.c
execute.o: execute.c
with_args.o: with_args.c
without_args.o: without_args.c

.PHONY: clean
clean:
	rm -f runner.o common.o execute.o with_args.o without_args.o
