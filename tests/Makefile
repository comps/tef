.PHONY: all
all: build

bin/ttee: bin/ttee.c

.PHONY: build
build: bin/ttee bin_clean
	$(MAKE) -C ../src clean && \
	CFLAGS="-Wall -O0 -g3" $(MAKE) -C ../src && \
	cp -v ../src/cli/ptef-{runner,report,mklog} ../src/lib/*.so bin/valgrind/cli/. && \
	$(MAKE) -C ../src clean && \
	$(MAKE) -C ../src && \
	cp -v ../src/cli/ptef-{runner,report,mklog} ../src/lib/*.so bin/cli/.

.PHONY: run
run:
	@./run

.PHONY: bin_clean
bin_clean:
	rm -f bin/valgrind/cli/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/cli/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/{ptef-{runner,report,mklog},libptef.so}

.PHONY: clean
clean: bin_clean
	rm -f valgrind.log stdout.log
	rm -f bin/ttee
	# TODO: clean logs/ dirs in all hierarchies
