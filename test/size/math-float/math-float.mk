#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# math-float.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/math-float.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a

TARGET_OBJ_LIBS := port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}

include ${PROJECT_ROOT}/tools/makefiles/project.mk

all: ${CURDIR}/math-float.elf

endif
