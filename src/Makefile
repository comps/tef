
#runner/runner.a:
#	$(MAKE) -C runner
#report/report.a:
#	$(MAKE) -C report
#cli/tef-runner: runner/runner.a report/report.a
#	$(MAKE) -C cli

.PHONY: all
all: lib cli

.PHONY: helpers runner report mklog lib cli
helpers:
	@$(MAKE) -C helpers
runner:
	@$(MAKE) -C runner
report:
	@$(MAKE) -C report
mklog:
	@$(MAKE) -C mklog
lib: helpers runner report mklog
	@$(MAKE) -C lib
cli: lib
	@$(MAKE) -C cli

.PHONY: clean
clean:
	@$(MAKE) -C helpers clean
	@$(MAKE) -C runner clean
	@$(MAKE) -C report clean
	@$(MAKE) -C mklog clean
	@$(MAKE) -C lib clean
	@$(MAKE) -C cli clean
