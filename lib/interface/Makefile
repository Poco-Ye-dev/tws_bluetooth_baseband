# SPDX-License-Identifier: MIT
objs := $(ROOT)/audio/interface/audio_api.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/interface.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
