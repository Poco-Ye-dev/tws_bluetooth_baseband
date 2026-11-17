# SPDX-License-Identifier: MIT
#
# Project selection and derived paths.
#
# config.mk is the only file a developer edits to choose a target.  Every
# variable below has a sane default so a fresh checkout builds without one.

CPU      ?= tcl34311
APP_CASE ?= earphone

CPU_DIR := $(CPU)
APP_DIR := $(ROOT)/apps/$(APP_CASE)
LIB_DIR := $(ROOT)/lib

CONFIG_FILE := $(ROOT)/config.mk

# ---------------------------------------------------------------------------
# Output layout
# ---------------------------------------------------------------------------
# Objects are mirrored below $(OUTPUT_PREFIX)$(ROOT) so a single checkout can
# produce debug and release artefacts without clobbering each other.
ifeq ($(cibuild),y)
  OUTPUT_PREFIX      :=
  POST_BUILD_SCRIPT  := $(ROOT)/cpu/$(CPU_DIR)/tools/download.bat
  CC_DEFINE          += -DCONFIG_RELEASE_ENABLE
else ifeq ($(HOST_OS),windows)
  OUTPUT_PREFIX      :=
  POST_BUILD_SCRIPT  := $(ROOT)\cpu\$(CPU_DIR)\tools\download.bat
else
  OUTPUT_PREFIX      := $(HOME)/tmp
  POST_BUILD_SCRIPT  := $(ROOT)/cpu/$(CPU_DIR)/tools/download.sh
  CC_DEFINE          += -D__SHELL__
endif

DIR_OUTPUT := $(ROOT)/cpu/$(CPU_DIR)/tools
AR_DIR     := $(ROOT)/cpu/$(CPU_DIR)/liba
AR_TARGET  := $(notdir $(wildcard $(LIB_DIR)/*/))
MAKE_RULE  := $(ROOT)/make/rules.mk

# Sub-makes (libraries and applications) see the same environment as the
# top-level make.  Keep this list in sync when a new global is introduced.
export ROOT CPU CPU_DIR APP_CASE APP_DIR LIB_DIR HOST_OS SLASH
export OUTPUT_PREFIX DIR_OUTPUT AR_DIR AR_TARGET MAKE_RULE
export OUTPUT_ELF LINKER ELF POST_BUILD_SCRIPT
export CC LD AR VER OBJCOPY OBJDUMP
export CC_DEFINE CC_ARGS LD_ARGS AR_ARGS SYS_INCLUDES SYS_LIBS
export includes V cibuild jtag

# ---------------------------------------------------------------------------
# Shared include path
# ---------------------------------------------------------------------------
includes += \
  -I$(ROOT)/include \
  -I$(ROOT)/include/core \
  -I$(ROOT)/include/driver \
  -I$(ROOT)/include/driver/cpu/$(CPU) \
  -I$(ROOT)/include/media \
  -I$(ROOT)/include/audio \
  -I$(ROOT)/include/bt \
  -I$(ROOT)/include/system \
  -I$(ROOT)/include/update \
  -I$(ROOT)/include/app \
  -I$(ROOT)/include/utils \
  -I$(ROOT)/include/interface \
  -I$(ROOT)/cpu/$(CPU_DIR) \
  -I$(APP_DIR) \
  -I$(APP_DIR)/include \
  -I$(ROOT)/apps/common/include \

ifeq ($(MEDIA_NEW_ENABLE),y)
includes += -I$(ROOT)/include/media/v2
else ifeq ($(MEDIA_NEW_ENABLE),d)
includes += -I$(ROOT)/include/media/devel
else
includes += -I$(ROOT)/include/media/v1
endif

# ---------------------------------------------------------------------------
# Platform description (toolchain, core, board, feature switches)
# ---------------------------------------------------------------------------
include $(ROOT)/make/platform/$(CPU).mk

# Every target may test CONFIG_CPU_<NAME> at compile time.
CC_DEFINE += -DCONFIG_CPU_$(shell echo $(CPU) | tr '[:lower:]' '[:upper:]')

# Per-application build description is appended last so it can extend objs,
# includes and LIBS.
-include $(APP_DIR)/app_cfg.mk
