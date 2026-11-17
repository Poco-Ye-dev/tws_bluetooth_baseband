# SPDX-License-Identifier: MIT
objs := $(ROOT)/media/codec/codec.o \
	$(ROOT)/media/codec/pcm.o \
	$(ROOT)/media/codec/sbc.o \
	$(ROOT)/media/pipeline/pipeline.o \
	$(ROOT)/media/container/container.o \
	$(ROOT)/media/core/frame.o \
	$(ROOT)/media/codec/adpcm.o \
	$(ROOT)/media/codec/sbc_hw.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/media.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
