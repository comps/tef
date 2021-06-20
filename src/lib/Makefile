.PHONY: all
all: libptef.so

# GNU LD
libptef.so: ../helpers/*.o ../runner/*.o ../report/*.o ../mklog/*.o
	$(CC) -shared -fPIC -Wl,--version-script=libptef.map -o $@ $^

.PHONY: clean
clean:
	rm -f libptef.so
