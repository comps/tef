all: runner.a

runner.a: runner.o execute.o with_args.o without_args.o
	ar -rcs $@ $^

CFLAGS += -I../include

runner.o: runner.c
execute.o: execute.c
with_args.o: with_args.c
without_args.o: without_args.c


.PHONY: clean
clean:
	rm -f runner.a runner.o execute.o with_args.o without_args.o
