.PHONY: all
all: tef-runner tef-report tef-mklog

CFLAGS += -I../include -I../helpers -Wall -g

tef-runner.o: tef-runner.c
tef-report.o: tef-report.c
tef-mklog.o: tef-mklog.c

tef-runner: tef-runner.o ../helpers/tef_helpers.o
	$(CC) -L../lib -o $@ $^ -ltef

tef-report: tef-report.o
	$(CC) -L../lib -o $@ $^ -ltef

tef-mklog: tef-mklog.o
	$(CC) -L../lib -o $@ $^ -ltef

.PHONY: clean
clean:
	rm -f tef-runner.o tef-report.o tef-mklog.o
	rm -f tef-runner tef-report tef-mklog
