.PHONY: all
all: libtef.so

# GNU LD
libtef.so: ../runner/*.o ../report/*.o
	$(CC) -shared -fPIC -Wl,--version-script=libtef.map -o $@ $^

.PHONY: clean
clean:
	rm -f libtef.so
