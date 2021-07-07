.PHONY: all
all: bash-builtins.so

CFLAGS ?= -O2 -Wall -g
CFLAGS += -I/usr/include/bash/include -I../include

bash-builtins.so: bash-builtins.c
	$(CC) -shared -fPIC -L../lib $(CFLAGS) -o $@ $^ -lptef

.PHONY: clean
clean:
	rm -f bash-builtins.so
