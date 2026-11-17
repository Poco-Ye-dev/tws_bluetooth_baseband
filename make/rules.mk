# SPDX-License-Identifier: MIT
#
# Generic compile/archive/link rules.
#
# A sub-make (an application or a library) lists sources in `objs`,
# `objs_cxx`, `obj_ls`, `obj_bs` and `objs_ver`, then includes this file.
# Object files keep the source suffix (.c.o / .cpp.o / .s.o / .S.o / .z.o)
# so mixed-language trees never collide.

objs     := $(abspath $(objs))
objs_cxx := $(abspath $(objs_cxx))
obj_ls   := $(abspath $(obj_ls))
obj_bs   := $(abspath $(obj_bs))
objs_ver := $(abspath $(objs_ver))

objs_out     := $(addprefix $(OUTPUT_PREFIX),$(objs:.o=.c.o))
objs_cxx_out := $(addprefix $(OUTPUT_PREFIX),$(objs_cxx:.o=.cpp.o))
obj_ls_out   := $(addprefix $(OUTPUT_PREFIX),$(obj_ls:.o=.s.o))
obj_bs_out   := $(addprefix $(OUTPUT_PREFIX),$(obj_bs:.o=.S.o))
objs_ver_out := $(addprefix $(OUTPUT_PREFIX),$(objs_ver:.o=.z.o))

obj_files := $(objs_out) $(objs_cxx_out) $(obj_ls_out) $(obj_bs_out) $(objs_ver_out)
dep_files := $(obj_files:.o=.d)

ifeq ($(cibuild),y)
  CC_ARGS += -w
endif

ifeq ($(TIDY_CHECK),y)
  _CC := $(TIDY_FILTER) $(CLANG_TIDY)
else
  _CC := $(CC)
endif

.PHONY: out archive object rm_lib clean dry_run
.SUFFIXES:

object: $(obj_files) rm_lib

out: object
	$(V) $(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) \
		$(ROOT)/cpu/$(CPU_DIR)/sdk_ld.c -o $(ROOT)/cpu/$(CPU_DIR)/sdk_ld.d
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P \
		$(ROOT)/cpu/$(CPU_DIR)/sdk_ld.c -o $(ROOT)/cpu/$(CPU_DIR)/sdk.ld
	$(V) $(LD) $(LD_ARGS) -o $(OUTPUT_ELF) $(obj_files) \
		--start-group $(LIBS) --end-group $(SYS_LIBS) $(LINKER)
ifeq ($(HOST_OS),windows)
	@cd $(DIR_OUTPUT) && $(POST_BUILD_SCRIPT) $(ELF)
else
ifeq ($(cibuild),y)
	@cd $(DIR_OUTPUT) && bash $(POST_BUILD_SCRIPT) $(ELF)
else
  ifeq ($(jtag),n)
	@cd $(DIR_OUTPUT) && bash $(POST_BUILD_SCRIPT) $(ELF)
  else
	@cd $(DIR_OUTPUT) && bash $(POST_BUILD_SCRIPT) $(ELF) _jtag
  endif
endif
endif

archive: object
	$(V) $(AR) $(AR_ARGS) $(AR_OUT) $(obj_files)

# Dependency scan is separate from compilation so a failing preprocessor does
# not leave a half-written object behind.
$(OUTPUT_PREFIX)%.z.o: %.z
	@echo + VER $<
	$(V) $(VER) $< $<.S
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $<.S -o $@

$(OUTPUT_PREFIX)%.s.o: %.s
	@echo + AS $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

$(OUTPUT_PREFIX)%.S.o: %.S
	@echo + AS $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

$(OUTPUT_PREFIX)%.c.o: %.c
	@echo + CC $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(_CC) $(SYS_INCLUDES) $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

$(OUTPUT_PREFIX)%.cpp.o: %.cpp
	@echo + CXX $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(_CC) $(SYS_INCLUDES) $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

rm_lib:
ifeq ($(GEN_LIB),y)
	@rm -f $(AR_OUT)
endif

dry_run: object

-include $(dep_files)
