.PHONY: all
all: ptef_helpers.o

CFLAGS ?= -O2 -flto -Wall -g
CFLAGS += -I../include -fPIC

ptef_helpers.o: ptef_helpers.c

.PHONY: clean
clean:
	rm -f ptef_helpers.o
