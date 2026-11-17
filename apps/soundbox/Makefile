# SPDX-License-Identifier: MIT
#
# Application image.  Shared by every product; APP_CASE selects the sources.

.DEFAULT_GOAL := out

objs := \
	$(APP_DIR)/main.o \

GEN_LIB := n
AR_OUT  :=

LIBS += \
	$(AR_DIR)/app.a \
	$(AR_DIR)/audio.a \
	$(AR_DIR)/bt.a \
	$(AR_DIR)/drivers.a \
	$(AR_DIR)/interface.a \
	$(AR_DIR)/kernel.a \
	$(AR_DIR)/media.a \
	$(AR_DIR)/system.a \
	$(AR_DIR)/update.a \
	$(AR_DIR)/utils.a \

include $(MAKE_RULE)

clean:
	@rm -f $(obj_files) $(dep_files)
