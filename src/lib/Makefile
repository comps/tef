.PHONY: all
all: libptef.so

CFLAGS ?= -O2 -flto -Wall -g

# GNU LD
libptef.so: ../helpers/*.o ../runner/*.o ../report/*.o ../mklog/*.o
	$(CC) -shared -fPIC -Wl,--version-script=libptef.map $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f libptef.so
