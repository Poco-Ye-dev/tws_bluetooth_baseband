# SPDX-License-Identifier: MIT
objs := $(ROOT)/cpu/components/components.o \
	$(ROOT)/cpu/config/config.o \
	$(ROOT)/cpu/periph_demo/periph_demo.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/system.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
