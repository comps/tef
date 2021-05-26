all: tef-runner

CFLAGS += -I$(PWD) -Wall -g

runner/runner.a:
	$(MAKE) -C runner
report/report.a:
	$(MAKE) -C report

tef-runner: tef-runner.o runner/runner.a report/report.a
	$(CC) -o $@ $^

tef-runner.o: tef-runner.c

.PHONY: clean
clean:
	$(MAKE) -C runner clean
	$(MAKE) -C report clean
	rm -f tef-runner tef-runner.o
