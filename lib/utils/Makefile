# SPDX-License-Identifier: MIT
objs := $(ROOT)/interface/utils/fs/fs.o \
	$(ROOT)/interface/utils/ui/ui.o \
	$(ROOT)/interface/utils/lzma/lzss.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/utils.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
