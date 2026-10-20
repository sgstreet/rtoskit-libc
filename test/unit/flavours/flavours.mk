#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# flavours.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/flavours.mk
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a

TARGET_OBJ_LIBS := port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}

include ${PROJECT_ROOT}/tools/makefiles/project.mk

# Exercise the library, not the compiler's built-in versions of the functions
CFLAGS += -fno-builtin

all: ${CURDIR}/flavours.elf ${CURDIR}/flavours-nofloat.elf ${CURDIR}/flavours-minimal.elf

# The same objects linked against each flavour archive instead of libc.a (PLAN D53)
flavour_libs = -Wl,--start-group ${1} $(shell ${CC} ${CROSS_FLAGS} -print-libgcc-file-name) -Wl,--end-group

${CURDIR}/flavours-nofloat.elf: ${OBJ} ${TARGET_OBJ} ${BUILD_ROOT}/libc/libc-nofloat.a ${EXTRA_DEPS}
	@echo "LINKING ${@}"
	$(LD) ${LDFLAGS} -Wl,-Map,"$(basename ${@}).map" ${LOADLIBES} -o $@ ${OBJ} ${TARGET_OBJ} $(call flavour_libs,${BUILD_ROOT}/libc/libc-nofloat.a)

${CURDIR}/flavours-minimal.elf: ${OBJ} ${TARGET_OBJ} ${BUILD_ROOT}/libc/libc-minimal-printf.a ${EXTRA_DEPS}
	@echo "LINKING ${@}"
	$(LD) ${LDFLAGS} -Wl,-Map,"$(basename ${@}).map" ${LOADLIBES} -o $@ ${OBJ} ${TARGET_OBJ} $(call flavour_libs,${BUILD_ROOT}/libc/libc-minimal-printf.a)

endif
