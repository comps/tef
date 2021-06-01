.PHONY: all
all: tef-runner tef-report

CFLAGS += -I../include -Wall -g

tef-runner.o: tef-runner.c
tef-report.o: tef-report.c

tef-runner: tef-runner.o ../runner/common.o
	$(CC) -L../lib -o $@ $^ -ltef

tef-report: tef-report.o
	$(CC) -L../lib -o $@ $^ -ltef

.PHONY: clean
clean:
	rm -f tef-runner.o tef-report.o
	rm -f tef-runner tef-report
