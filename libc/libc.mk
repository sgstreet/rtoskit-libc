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

# Builds libc.a from src/ and the arch layers (see tools/makefiles/libc.mk)
# and installs it with the public headers.

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/libc.mk ${SOURCE_DIR}/subdir.mk

include ${PROJECT_ROOT}/tools/makefiles/project.mk

LIBC_HEADERS := $(shell cd ${PROJECT_ROOT}/include && find . -name '*.h')
LIBC_ARCH_HEADERS := $(shell cd ${PROJECT_ROOT}/arch/${ARCH_FAMILY}/include && find . -name '*.h')
LIBC_INSTALLED_HEADERS := $(addprefix ${INSTALL_ROOT}/include/,$(patsubst ./%,%,${LIBC_HEADERS} ${LIBC_ARCH_HEADERS}))

all: ${INSTALL_ROOT}/lib/libc.a ${LIBC_INSTALLED_HEADERS}

${CURDIR}/libc.a: ${OBJ} ${EXTRA_DEPS}
	$(call libc-archive,${OBJ})

${INSTALL_ROOT}/lib/libc.a: ${CURDIR}/libc.a
	$(call libc-install,$<,$@)

${INSTALL_ROOT}/include/%.h: ${PROJECT_ROOT}/arch/${ARCH_FAMILY}/include/%.h
	$(call libc-install,$<,$@)

${INSTALL_ROOT}/include/%.h: ${PROJECT_ROOT}/include/%.h
	$(call libc-install,$<,$@)

endif
