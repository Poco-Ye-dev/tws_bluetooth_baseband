# SPDX-License-Identifier: MIT
#
# Application specific options.  Included by make/project.mk after the
# platform description, so it can add defines, include paths and libraries.

includes += \
	-I$(APP_DIR) \
	-I$(APP_DIR)/board/$(CPU) \

LIBS += \
	$(AR_DIR)/kernel.a \
