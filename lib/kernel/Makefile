# SPDX-License-Identifier: MIT
objs := $(ROOT)/kernel/lib/string.o \
	$(ROOT)/kernel/lib/alloc.o \
	$(ROOT)/kernel/lib/printk.o \
	$(ROOT)/kernel/lib/ringbuf.o \
	$(ROOT)/kernel/lib/crc.o \
	$(ROOT)/kernel/lib/math.o \
	$(ROOT)/kernel/lib/crypto.o \
	$(ROOT)/kernel/lib/e0.o \
	$(ROOT)/kernel/os/task.o \
	$(ROOT)/kernel/os/sync.o \
	$(ROOT)/kernel/os/timer.o \
	$(ROOT)/kernel/os/event.o \
	$(ROOT)/cpu/$(CPU_DIR)/port.o \
	$(ROOT)/kernel/os/init.o \
	$(ROOT)/kernel/os/workqueue.o \
	$(ROOT)/kernel/lib/log.o \
	$(ROOT)/cpu/$(CPU_DIR)/clock.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/kernel.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
