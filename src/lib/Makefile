.PHONY: all
all: libtef.so

# GNU LD
libtef.so: ../runner/runner.a ../report/report.a
	$(CC) -shared -fPIC -Wl,--version-script=libtef.map -o $@ $<

.PHONY: clean
clean:
	rm -f libtef.so
