all: runner.a

runner.a: runner.o common.o execute.o with_args.o without_args.o
	ar -rcs $@ $^

CFLAGS += -Wall -I.. -g

runner.o: runner.c
common.o: common.c
execute.o: execute.c
with_args.o: with_args.c
without_args.o: without_args.c


.PHONY: clean
clean:
	rm -f runner.a runner.o common.o execute.o with_args.o without_args.o
