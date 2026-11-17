# SPDX-License-Identifier: MIT
#
# Top-level build entry.
#
#   make                      build the application selected by APP_CASE
#   make app                  same as `make`
#   make libs                 build every library below lib/
#   make lib TARGET=<name>    build a single library
#   make clean                remove intermediate files
#   make debug                attach GDB to the last built image
#
# All project options live in config.mk; this file only consumes them.

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include $(ROOT)/make/host.mk
-include $(ROOT)/config.mk
include $(ROOT)/make/project.mk
include $(ROOT)/make/rules.mk

.DEFAULT_GOAL := all

.PHONY: all app libs lib clean clean_libs clean_lib debug usage

all: prebuild $(ADDITION_DEP)
	$(MAKE) -C $(APP_DIR)

app: all

libs: prebuild
	$(MAKE) -C $(ROOT)/lib archive

lib: prebuild
	$(MAKE) -C $(ROOT)/lib/$(TARGET) archive

clean:
	@find $(OUTPUT_PREFIX)$(ROOT) -name '*.d' -delete
	@find $(OUTPUT_PREFIX)$(ROOT) -name '*.o' -delete
	@find $(OUTPUT_PREFIX)$(ROOT) -name '*.z.S' -delete

clean_libs:
	@find $(ROOT)/lib -name '*.a' -delete

clean_lib:
	@rm -f $(AR_DIR)/$(TARGET).a

debug: $(OUTPUT_ELF)
	$(SHELL) $(DIR_OUTPUT)/run_jtag.sh $<

prebuild:
ifneq ($(cibuild),y)
	@mkdir -p $(OUTPUT_PREFIX)$(ROOT)
endif

usage:
	@echo "build application : make [APP_CASE=...] [CPU=...]"
	@echo "build libraries   : make libs"
	@echo "build one library : make lib TARGET=utils"
	@echo "clean             : make clean"
	@echo "debug             : make debug"

include $(ROOT)/make/local.mk
