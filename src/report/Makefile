.PHONY: all
all: report.o

CFLAGS ?= -O2 -Wall -g
CFLAGS += -I../include -I../helpers -fPIC

report.o: report.c

.PHONY: clean
clean:
	rm -f report.o
