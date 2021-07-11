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
	cp -v ../src/cli/ptef-{runner,report,mklog} ../src/lib/*.so bin/cli/. && \
	cp -v ../src/bash/bash-builtins.{so,sh} bin/bash/.

.PHONY: run
run:
	@./run

.PHONY: bin_clean
bin_clean:
	rm -f bin/valgrind/cli/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/cli/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/{ptef-{runner,report,mklog},libptef.so}
	rm -f bin/bash/bash-builtins.{so,sh}

.PHONY: clean
clean: bin_clean
	rm -f valgrind.log stdout-{cli,valgrind,bash,python}.log
	rm -f bin/ttee
	find . -depth -type d \( -name logs -o -name testlogs \) -exec rm -rf {} \;
	find . -depth -type f \( -name core.\* -o -name vgcore.\* \) -exec rm -f {} \;
