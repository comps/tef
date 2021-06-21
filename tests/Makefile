.PHONY: all
all: build

.PHONY: build
build: clean
	$(MAKE) -C ../src clean && \
	CFLAGS="-Wall -O0 -g3" $(MAKE) -C ../src && \
	cp -v ../src/cli/ptef-{runner,report,mklog} ../src/lib/*.so bin/valgrind/cli/. && \
	$(MAKE) -C ../src clean && \
	$(MAKE) -C ../src && \
	cp -v ../src/cli/ptef-{runner,report,mklog} ../src/lib/*.so bin/cli/.

.PHONY: run
run:
	@./run

.PHONY: clean
clean:
	rm -f bin/valgrind/cli/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/cli/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/{ptef-{runner,report,mklog},libptef.so}
	rm -f valgrind.log stdout.log
