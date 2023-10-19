MODULES = fetcher packager
MODULE_PATHS = $(foreach module,$(MODULES),$(abspath $(module)))

all: $(MODULES)
	for module in $(MODULE_PATHS); do $(MAKE) all -C $$module; done
	$(info All modules built!)

clean:
	for module in $(MODULE_PATHS); do $(MAKE) clean -C $$module; done
	$(info All modules cleaned!)

update:
	git submodule update --recursive --remote
