# SPDX-License-Identifier: MIT
objs := $(ROOT)/audio/effect/effect.o \
	$(ROOT)/audio/path/path.o \
	$(ROOT)/audio/anc/anc.o \
	$(ROOT)/audio/cvp/cvp.o \
	$(ROOT)/audio/kws/kws.o \
	$(ROOT)/audio/effect/dsp.o \
	$(ROOT)/audio/effect/eq.o \
	$(ROOT)/audio/common/common.o \
	$(ROOT)/audio/framework/framework.o \
	$(ROOT)/audio/le_audio/le_audio.o \
	$(ROOT)/audio/smart_voice/smart_voice.o \
	$(ROOT)/audio/test_tools/test_tools.o \
	$(ROOT)/audio/platform/platform.o \
	$(ROOT)/audio/cpu/audio_cpu.o \
	$(ROOT)/audio/cpu/$(CPU_DIR)/audio_port.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/audio.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
