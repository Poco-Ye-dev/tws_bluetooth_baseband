# SPDX-License-Identifier: MIT
#
# Platform description for tcl77334.
#
# This file answers three questions: which core, which board, and which
# compile-time feature switches are enabled.  Toolchain paths and code
# generation flags are supplied by make/toolchain.mk.

CORE         := pi32v2
BOARD        := generic
TYPE         ?= CHIP
RTOS         ?= UCOS
LARGE_PROGRAM ?= y

ifeq ($(LARGE_PROGRAM),y)
  COMPILE_MODE := lto_r3_large
else
  COMPILE_MODE := lto_r3
endif

# ---------------------------------------------------------------------------
# Feature switches
# ---------------------------------------------------------------------------
NEW_BREDR               ?= y
NEW_MODEM               ?= y
USER_AFH_MODEM          ?= n
BT_AI_EN                ?= n
NEW_ECC                 ?= y
WIFI_DETECT_ENABLE      ?= y
EX_TWS_ADJUST_ENABLE    ?= y
MOVABLE_ENABLE          ?= n
NEW_TWS_BREDR_FORWARD   ?= y
LE_AUDIO_SUPPORT        ?= y
THIRD_PARTY_PROFILE_ENABLE ?= y

CC_DEFINE += \
  -DCONFIG_USB_SUPPORT_MRX_TX \
  -DCONFIG_PRINT_IN_MASK

ifeq ($(NEW_BREDR),y)
  CC_DEFINE += -DCONFIG_NEW_BREDR_ENABLE
endif
ifeq ($(NEW_MODEM),y)
  CC_DEFINE += -DCONFIG_NEW_MODEM_ENABLE
endif
ifeq ($(USER_AFH_MODEM),y)
  CC_DEFINE += -DCONFIG_USER_AFH_ENABLE
endif
ifeq ($(NEW_TWS_BREDR_FORWARD),y)
  CC_DEFINE += -DCONFIG_NEW_TWS_FORWARD_ENABLE
endif
ifeq ($(BT_AI_EN),y)
  CC_DEFINE += -DCONFIG_APP_BT_ENABLE
endif
ifeq ($(NEW_ECC),y)
  CC_DEFINE += -DCONFIG_NEW_ECC_ENABLE
endif
ifeq ($(EX_TWS_ADJUST_ENABLE),y)
  CC_DEFINE += -DCONFIG_SUPPORT_EX_TWS_ADJUST
endif
ifeq ($(TYPE),FPGA)
  CC_DEFINE += -DCONFIG_FPGA_ENABLE
endif

ifeq ($(RTOS),FREE_RTOS)
  CC_DEFINE += -DCONFIG_FREE_RTOS_ENABLE
else ifeq ($(RTOS),UCOS)
  CC_DEFINE += -DCONFIG_UCOS_ENABLE
endif

# Audio / media capabilities of this part.
CC_DEFINE += \
  -DCONFIG_AUDIO_ONCHIP \
  -DCONFIG_EQ_SUPPORT_ASYNC \
  -DCONFIG_EQ_CORE_V1 \
  -DCONFIG_SBC_CODEC_HW \
  -DCONFIG_MSBC_CODEC_HW \
  -DCONFIG_AAC_CODEC_FFT_USE_MUTEX \
  -DCONFIG_SIN_TONE_V2

ifeq ($(WIFI_DETECT_ENABLE),y)
  CC_DEFINE += -DCONFIG_SUPPORT_WIFI_DETECT
endif

# ---------------------------------------------------------------------------
# Image layout
# ---------------------------------------------------------------------------
ELF        := sdk
BUILD_LD   := sdk.ld
BUILD_MAP  := sdk.map
OUTPUT_ELF := $(DIR_OUTPUT)/$(ELF).elf

LINKER := \
  -T$(ROOT)/cpu/$(CPU_DIR)/$(BUILD_LD) \
  -M=$(DIR_OUTPUT)/$(BUILD_MAP)

include $(ROOT)/make/toolchain.mk
