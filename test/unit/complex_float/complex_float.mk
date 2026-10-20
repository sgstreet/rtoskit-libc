#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# complex_float.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/complex_float.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a

TARGET_OBJ_LIBS := port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}

# As float_accuracy (PLAN D57): on x86_64 the float complex layer and the float-float libm layer it
# stands on are compiled into the test, so that many arguments run natively; elsewhere the test
# measures the library as built
ifeq (${ARCH_FAMILY},x86_64)
LAYER_SRC := $(wildcard ${PROJECT_ROOT}/libc/math/sp-nearest/*.c ${PROJECT_ROOT}/libc/complex/sp-nearest/*.c)
EXTRA_OBJS := $(patsubst ${PROJECT_ROOT}/libc/%.c,${CURDIR}/layer/%.o,${LAYER_SRC})
SAMPLES := $(if $(filter debug,${BUILD_TYPE}),16384,262144)
LAYER := 1
else
SAMPLES := 2048
LAYER := $(if $(filter sp-nearest,${LIBC_MATH_LAYERS}),1,0)
endif

include ${PROJECT_ROOT}/tools/makefiles/project.mk

# Exercise the library, not the compiler's built-in versions of the functions
CFLAGS += -fno-builtin
CPPFLAGS += -I${PROJECT_ROOT}/libc -DCOMPLEX_FLOAT_SAMPLES=${SAMPLES} -DCOMPLEX_FLOAT_LAYER=${LAYER}

${CURDIR}/layer/%.o: ${PROJECT_ROOT}/libc/%.c ${EXTRA_DEPS}
	@echo "COMPILING ${<}"
	@mkdir -p $(dir $@)
	${CC} ${CPPFLAGS} ${CFLAGS} -MMD -MP -c -o $@ $<

-include ${EXTRA_OBJS:.o=.d}

all: ${CURDIR}/complex_float.elf

endif
