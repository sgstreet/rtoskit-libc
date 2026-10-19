#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# libc.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

# libc.a collects the objects of every libc module, including the ${ARCH_CROSS} and ${ARCH_FAMILY} variants
# and the allocator variant named by ${LIBC_MALLOC} (set by the board file, PLAN D32)
$(if $(wildcard ${PROJECT_ROOT}/libc/malloc/${LIBC_MALLOC}/${LIBC_MALLOC}.mk),,$(error LIBC_MALLOC='${LIBC_MALLOC}' is not an allocator in libc/malloc))
TARGET_OBJ_LIBS := libc/assert
TARGET_OBJ_LIBS += libc/ctype
TARGET_OBJ_LIBS += libc/decimal
TARGET_OBJ_LIBS += libc/errno
TARGET_OBJ_LIBS += libc/fenv/${ARCH_CROSS}
TARGET_OBJ_LIBS += libc/inttypes
TARGET_OBJ_LIBS += libc/locale
TARGET_OBJ_LIBS += libc/malloc
TARGET_OBJ_LIBS += libc/malloc/${LIBC_MALLOC}
TARGET_OBJ_LIBS += libc/setjmp/${ARCH_CROSS}
TARGET_OBJ_LIBS += libc/signal
TARGET_OBJ_LIBS += libc/startup
TARGET_OBJ_LIBS += libc/stdbit
TARGET_OBJ_LIBS += libc/stdio
TARGET_OBJ_LIBS += libc/stdlib
TARGET_OBJ_LIBS += libc/string
TARGET_OBJ_LIBS += libc/time
TARGET_OBJ_LIBS += libc/tls/${ARCH_FAMILY}

EXTRA_DEPS := ${SOURCE_DIR}/libc.mk

LIBC_HEADERS := $(patsubst ${PROJECT_ROOT}/include/%,${INSTALL_ROOT}/include/%,$(shell find ${PROJECT_ROOT}/include -name '*.h'))

include ${PROJECT_ROOT}/tools/makefiles/project.mk

all: ${INSTALL_ROOT}/lib/libc.a ${LIBC_HEADERS}

${INSTALL_ROOT}/lib/libc.a: ${CURDIR}/libc.a
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 644 -D ${<} ${@}

${INSTALL_ROOT}/include/%.h: ${PROJECT_ROOT}/include/%.h
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 644 -D ${<} ${@}

endif
