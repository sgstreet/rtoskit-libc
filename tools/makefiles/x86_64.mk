#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# x86_64.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# x86_64 Linux target using rtoskit-libc (port/linux) instead of the host libc

CC := gcc-14
CXX := g++-14
LD := gcc-14
AR := ar
AS := as
OBJCOPY := objcopy
OBJDUMP := objdump
SIZE := size
NM := nm
INSTALL := install
INSTALL_STRIP := install --strip-program=strip -s

ARCH_FAMILY := x86_64
ARCH_LAYERS := x86_64
RTOS_PORT ?= linux

CPPFLAGS :=
ARFLAGS := cr
ASFLAGS :=

# -mlong-double-64 matches long double to the ARM targets (PLAN D7)
CFLAGS := -std=c23 -pipe -fmessage-length=0 -ffunction-sections -fdata-sections -fno-common -fno-pie -fno-stack-protector -mlong-double-64 -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror
CXXFLAGS :=

LDSCRIPTS :=
LDFLAGS := -static -no-pie -Wl,--gc-sections
LOADLIBES :=
LDLIBS :=

LISTING_STYLE := -adlns

include ${PROJECT_ROOT}/tools/makefiles/libc-target.mk
