.PHONY: all clean

all: bin lib

bin:
	$(MAKE) -C src
	mkdir $@
	install -m 755 src/runner $@/tef-runner
	install -m 755 src/report $@/tef-report
	install -m 755 src/mklog  $@/tef-mklog

lib:
	mkdir $@
	install -m 644 src/common.inc $@/tef.bash

clean:
	rm -rf bin lib
	$(MAKE) -C src clean
