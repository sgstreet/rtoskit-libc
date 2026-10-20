#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# hello-printf-int-minimal.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/hello-printf-int-minimal.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc-minimal-printf.a

TARGET_OBJ_LIBS := port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}

include ${PROJECT_ROOT}/tools/makefiles/project.mk

# Linked against libc-minimal-printf.a instead of libc.a (PLAN D53)
LDLIBS := -Wl,--start-group ${BUILD_ROOT}/libc/libc-minimal-printf.a $(shell ${CC} ${CROSS_FLAGS} -print-libgcc-file-name) -Wl,--end-group

all: ${CURDIR}/hello-printf-int-minimal.elf

endif
