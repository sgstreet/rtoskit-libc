#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# none.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

# Linked with librtos-none.a instead of the board's port, as a bare-metal image would be
EXTRA_DEPS := ${SOURCE_DIR}/none.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a ${BUILD_ROOT}/port/none/librtos-none.a

include ${PROJECT_ROOT}/tools/makefiles/project.mk

# Exercise the library, not the compiler's built-in versions of the functions
CFLAGS += -fno-builtin

# rtos-none.h, and the ELF structures to find the TLS image
CPPFLAGS += -I${PROJECT_ROOT}/port/none -I${PROJECT_ROOT}/port/linux/${ARCH_FAMILY}

# The port archive joins libc.a's group; its heap is a 64 KiB array in the test
comma := ,
LDLIBS := -Wl,--start-group ${BUILD_ROOT}/port/none/librtos-none.a $(filter-out -Wl${comma}--start-group,${LDLIBS})
LDFLAGS += -Wl,--defsym=__rtos_none_heap_end=__rtos_none_heap_start+65536

all: ${CURDIR}/none.elf

endif
