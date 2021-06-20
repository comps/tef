.PHONY: all
all: mklog.o

CFLAGS ?= -O2 -Wall -g
CFLAGS += -I../include -I../helpers -fPIC

mklog.o: mklog.c

.PHONY: clean
clean:
	rm -f mklog.o
