#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# softdp.mk
#
# Created on: Oct 22, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/softdp.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a

TARGET_OBJ_LIBS := port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}

# On x86_64 the integer double-precision helpers of the soft-dp layer are compiled into the test, so that
# the comparison with the hardware runs natively (PLAN D99)
ifeq (${ARCH_FAMILY},x86_64)
SOFTDOUBLE_SRC := $(wildcard ${PROJECT_ROOT}/libc/math/soft-dp/*.c)
EXTRA_OBJS := $(patsubst ${PROJECT_ROOT}/libc/math/soft-dp/%.c,${CURDIR}/soft-dp/%.o,${SOFTDOUBLE_SRC})
endif

include ${PROJECT_ROOT}/tools/makefiles/project.mk

# Exercise the library, not the compiler's built-in versions of the functions
CFLAGS += -fno-builtin

# The helpers are internal to the library; their base calling convention attribute is Arm's only
CPPFLAGS += -I${PROJECT_ROOT}/libc -I${PROJECT_ROOT}/libc/math/soft-dp
CFLAGS += -Wno-attributes


${CURDIR}/soft-dp/%.o: ${PROJECT_ROOT}/libc/math/soft-dp/%.c ${EXTRA_DEPS}
	@echo "COMPILING ${<}"
	@mkdir -p $(dir $@)
	${CC} ${CPPFLAGS} ${CFLAGS} -MMD -MP -c -o $@ $<

-include ${EXTRA_OBJS:.o=.d}

all: ${CURDIR}/softdp.elf

endif
