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

# x86_64 toolchain building against rtoskit-libc instead of the host C library

CROSS_FLAGS ?= -mlong-double-64

export ARCH_FAMILY := x86_64
# <fenv.h> implementation: x86_64, fpv5-dp (every floating type in the FPU), or soft (D44)
export LIBC_FENV := x86_64
# libm functions in FPU instructions: libc/math/<layer> replaces the generic files of the same name (D46)
export LIBC_MATH_LAYERS := x86_64

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

# The predefined feature macros a hosted compiler would take from <stdc-predef.h> (D50)
CPPFLAGS := -nostdinc -isystem ${PROJECT_ROOT}/include -include ${PROJECT_ROOT}/include/stdc-predef.h
ARFLAGS := cr
ASFLAGS :=

# -mlong-double-64 makes long double binary64 as on the ARM targets (PLAN D7)
# -muclibc: the host compiler assumes glibc and would fuse sin and cos into sincos, which C does not
# reserve and rtoskit-libc does not provide; uClibc is the GNU/Linux C library it knows without sincos (D51)
# No unwind tables: C needs none, and .eh_frame was 28 % of libc.a (P2.02, D52)
# Frame sizes and call graphs for tools/scripts/size-report (D51)
CFLAGS := ${CROSS_FLAGS} -muclibc -fno-asynchronous-unwind-tables -fno-unwind-tables -pipe -feliminate-unused-debug-types -fmessage-length=0 -ffunction-sections -fdata-sections -fno-common -fno-pie -fno-stack-protector -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=c23 -fstack-usage -fcallgraph-info=su
CXXFLAGS :=

# Added by the modules that build the library and the ports
LIBC_CFLAGS := -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -ftls-model=local-exec

LDSCRIPTS :=
LDFLAGS := ${CROSS_FLAGS} -static -no-pie -nostdlib -Wl,--gc-sections
LOADLIBES :=
LDLIBS := -Wl,--start-group ${BUILD_ROOT}/libc/libc.a $(shell ${CC} ${CROSS_FLAGS} -print-libgcc-file-name) -Wl,--end-group

# As for host builds in release.mk: the native assembler cannot interleave source (-h) in listings of optimised code
LISTING_STYLE := -adlns
