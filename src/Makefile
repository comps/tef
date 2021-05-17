all: prog

CFLAGS += -Iinclude -Wall -g

prog: cli.o runner/runner.a
	$(CC) -o $@ $^

cli.o: cli.c

runner/runner.a:
	$(MAKE) -C runner


.PHONY: clean
clean:
	$(MAKE) -C runner clean
	rm -f prog cli.o
