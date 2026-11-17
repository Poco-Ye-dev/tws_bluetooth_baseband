# SPDX-License-Identifier: MIT
#
# pi32 / pi32v2 toolchain description.
#
# A platform file sets CORE and COMPILE_MODE before including this file.
# Only the CORE == pi32v2 combination is described here; add a new branch
# when a new core is introduced instead of scattering conditionals.

TCL_TOOLS ?= /opt/pi32v2

ifeq ($(HOST_OS),linux)
  CC      := /opt/reltool/pi32v2/bin/clang
  LD      := $(TCL_TOOLS)/bin/lto-wrapper
  AR      := $(TCL_TOOLS)/bin/lto-ar
  VER     := /opt/utils/replacetime
  OBJCOPY := $(TCL_TOOLS)/bin/pi32v2-uclinux-objcopy
  OBJDUMP := $(TCL_TOOLS)/bin/pi32v2-uclinux-objdump
  SYS_INCLUDES := -I$(TCL_TOOLS)/newlib/include
else
  TCL_TOOLS ?= C:/TCL/pi32
  CC      := $(TCL_TOOLS)/bin/clang.exe
  LD      := $(TCL_TOOLS)/bin/pi32v2-lto-wrapper.exe
  AR      := $(TCL_TOOLS)/bin/pi32v2-lto-ar.exe
  VER     := $(TCL_TOOLS)/bin/replacetime.exe
  OBJCOPY := $(TCL_TOOLS)/bin/llvm-objcopy.exe
  OBJDUMP := $(TCL_TOOLS)/bin/llvm-objdump.exe
  SYS_INCLUDES := -I$(TCL_TOOLS)/pi32v2-include
endif

# ---------------------------------------------------------------------------
# Code generation
# ---------------------------------------------------------------------------
CC_ARGS += \
  -target pi32v2 -mcpu=r3 \
  -mllvm -pi32v2-merge-max-offset=4096 \
  -mllvm -pi32v2-enable-simd=true \
  -mllvm -inline-threshold=5 \
  -mllvm -inline-max-allocated-size=32 \
  -fno-common \
  -integrated-as \
  -Oz -c -g -flto \
  -fallow-pointer-null \
  -fprefer-gnu-section \
  -Wframe-larger-than=256 \
  -Wreturn-type \
  -Werror

ifeq ($(FLOAT_DEBUG),y)
  CC_ARGS += -DCONFIG_FLOAT_DEBUG_ENABLE -mllvm -floating-point-may-trap=true
  LD_ARGS += --plugin-opt=-floating-point-may-trap=true
endif

ifeq ($(LARGE_PROGRAM),y)
  CC_ARGS += -mllvm -pi32v2-large-program=true
  LD_ARGS += --plugin-opt=-pi32v2-large-program=true
endif

# ---------------------------------------------------------------------------
# Linker
# ---------------------------------------------------------------------------
LD_ARGS += \
  -L$(TCL_TOOLS)/lib \
  --plugin-opt=-pi32v2-always-use-itblock=false \
  --plugin-opt=-enable-ipra=true \
  --plugin-opt=-pi32v2-merge-max-offset=4096 \
  --plugin-opt=-pi32v2-enable-simd=true \
  --plugin-opt=mcpu=r3 \
  --plugin-opt=-global-merge-on-const \
  --plugin-opt=-inline-threshold=5 \
  --plugin-opt=-inline-max-allocated-size=32 \
  --plugin-opt=-inline-normal-into-special-section=true \
  --plugin-opt=-pi32v2-enable-rep-memop \
  --plugin-opt=-warn-stack-size=256 \
  --sort-common \
  --gc-sections

AR_ARGS += -r -s
