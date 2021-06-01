.PHONY: all
all: report.a

report.a: report.o
	ar -rcs $@ $^

CFLAGS += -I../include -fPIC -Wall -g

report.o: report.c

.PHONY: clean
clean:
	rm -f report.a report.o
