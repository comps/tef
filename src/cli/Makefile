.PHONY: all
all: tef-runner tef-report

tef-runner.o: tef-runner.c
tef-report.o: tef-report.c

tef-runner: tef-runner.o ../runner/runner.a ../report/report.a
	$(CC) -o $@ $^

tef-report: tef-report.o ../report/report.a
	$(CC) -o $@ $^

.PHONY: clean
clean:
	rm -f tef-runner.o tef-report.o
	rm -f tef-runner tef-report
