all: tef-runner

CFLAGS += -I$(PWD) -Wall -g

runner/runner.a:
	$(MAKE) -C runner

tef-runner: tef-runner.o runner/runner.a
	$(CC) -o $@ $^

tef-runner.o: tef-runner.c

.PHONY: clean
clean:
	$(MAKE) -C runner clean
	rm -f tef-runner tef-runner.o
