export CFLAGS += -I$(PWD)/include -Wall -g

#runner/runner.a:
#	$(MAKE) -C runner
#report/report.a:
#	$(MAKE) -C report
#cli/tef-runner: runner/runner.a report/report.a
#	$(MAKE) -C cli

.PHONY: all
all: cli lib

.PHONY: runner report cli lib
runner:
	@$(MAKE) -C runner
report:
	@$(MAKE) -C report
cli: runner report
	@$(MAKE) -C cli
lib: runner report
	@$(MAKE) -C lib

.PHONY: clean
clean:
	@$(MAKE) -C runner clean
	@$(MAKE) -C report clean
	@$(MAKE) -C cli clean
	@$(MAKE) -C lib clean
