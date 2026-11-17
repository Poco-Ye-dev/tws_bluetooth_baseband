# SPDX-License-Identifier: MIT
#
# Host environment detection.  Kept intentionally small: the toolchain
# itself is described by make/platform/<cpu>.mk, not by the host OS.

ifeq ($(shell uname -s),Linux)
  HOST_OS      := linux
  SLASH        := /
  ADDITION_DEP := libs
else
  HOST_OS      := windows
  SLASH        := \\
  ADDITION_DEP :=
endif

# Verbosity.  Set V= to see every command.
V ?= @
