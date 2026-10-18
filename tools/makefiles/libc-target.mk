#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# libc-target.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Compile and link everything built for a target against rtoskit-libc from
# the build tree. Included at the end of each <arch>.mk, after ARCH_FAMILY,
# ARCH_LAYERS and RTOS_PORT are set.

LIBC_INCLUDE_DIRS := ${PROJECT_ROOT}/arch/${ARCH_FAMILY}/include ${PROJECT_ROOT}/include

LIBC_ARCHIVE := ${BUILD_ROOT}/libc/libc.a
LIBC_PORT_DIR := ${BUILD_ROOT}/port/${RTOS_PORT}
LIBC_PORT_ARCHIVE := ${LIBC_PORT_DIR}/librtos-${RTOS_PORT}.a
LIBC_CRT := ${LIBC_PORT_DIR}/crt1.o
LIBC_LIBGCC := $(shell ${CC} ${CFLAGS} -print-libgcc-file-name)

CPPFLAGS += -nostdinc $(addprefix -isystem ,${LIBC_INCLUDE_DIRS})
LDFLAGS += -nostdlib
LOADLIBES += ${LIBC_CRT}
LDLIBS += -Wl,--start-group ${LIBC_ARCHIVE} ${LIBC_PORT_ARCHIVE} ${LIBC_LIBGCC} -Wl,--end-group

# Relink executables when the libc or port libraries change
EXTRA_ELF_DEPS += ${LIBC_CRT} ${LIBC_ARCHIVE} ${LIBC_PORT_ARCHIVE}
