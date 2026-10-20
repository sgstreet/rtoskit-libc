#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# malloc-tiny.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#


# bench-malloc linked against libc-tinymalloc.a instead of libc.a (PLAN D54)

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/malloc-tiny.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc-tinymalloc.a

TARGET_OBJ_LIBS := test/bench/malloc test/bench/harness test/bench/harness/${ARCH_FAMILY} port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}

include ${PROJECT_ROOT}/tools/makefiles/project.mk

LDLIBS := -Wl,--start-group ${BUILD_ROOT}/libc/libc-tinymalloc.a $(shell ${CC} ${CROSS_FLAGS} -print-libgcc-file-name) -Wl,--end-group

all: ${CURDIR}/bench-malloc-tiny.elf

endif
