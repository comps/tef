all: report.a

report.a: report.o
	ar -rcs $@ $^

CFLAGS += -Wall -I.. -g

report.o: report.c

.PHONY: clean
clean:
	rm -f report.a report.o
