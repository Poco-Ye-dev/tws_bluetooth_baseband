# SPDX-License-Identifier: MIT
objs := $(ROOT)/update/ota.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/update.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
