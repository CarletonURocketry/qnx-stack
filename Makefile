MODULES = fetcher packager broadcaster plogger
MODULE_PATHS = $(foreach module,$(MODULES),$(abspath $(module)))
PI_ARCH = nto-aarch64-o.le
UTILITIES = i2c-scanner eeprom-write
BINARIES = $(foreach module,$(MODULES) $(UTILITIES),$(abspath $(module))/$(PI_ARCH)/$(module))

# Port is optional but will be 22 by default
deploy: PORT = 22

# Modules should not be treated as having dependencies
.PHONY: $(MODULES)
.PHONY: $(UTILITIES)

# Calls Makefile for the specific project
$(MODULES):
	$(info Building $@)
	$(MAKE) all -C $(abspath $@)

# Compiles all binaries
all: $(MODULES)
	$(info All modules built!)

$(UTILITIES):
	$(info Building $@)
	$(MAKE) all -C $(abspath $@)

# Cleans all binaries
clean:
	for module in $(MODULE_PATHS); do $(MAKE) clean -C $$module; done
	$(info All modules cleaned!)

# Updates all sub modules recursively
update:
	git pull
	git submodule foreach git pull origin main; git checkout main

# Rule for uploading binaries to the Raspberry Pi
deploy:
ifeq ($(HOST),)
	$(error Try using 'make deploy HOST=<host>')
endif

ifeq ($(OS),Windows_NT)
	deploy.bat $(HOST) $(PORT) $(BINARIES)
else
	./deploy.sh -h $(HOST) -p $(PORT) $(BINARIES)
endif
