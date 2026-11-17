# SPDX-License-Identifier: MIT
objs := $(ROOT)/apps/common/app.o \
	$(ROOT)/apps/common/key.o \
	$(ROOT)/apps/common/power.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/app.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
