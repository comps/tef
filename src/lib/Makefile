.PHONY: all
all: libtef.so

# GNU LD
libtef.so: ../helpers/*.o ../runner/*.o ../report/*.o ../mklog/*.o
	$(CC) -shared -fPIC -Wl,--version-script=libtef.map -o $@ $^

.PHONY: clean
clean:
	rm -f libtef.so
