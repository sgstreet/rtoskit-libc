#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# none.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Builds the none RTOS port: librtos-none.a plus the startup object crt1.o

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/none.mk ${SOURCE_DIR}/subdir.mk

include ${PROJECT_ROOT}/tools/makefiles/project.mk

PORT_CRT_OBJ := $(filter %/crt1.o,${OBJ})
PORT_OBJ := $(filter-out %/crt1.o,${OBJ})

all: ${INSTALL_ROOT}/lib/librtos-none.a $(if ${PORT_CRT_OBJ},${INSTALL_ROOT}/lib/none/crt1.o)

${CURDIR}/librtos-none.a: ${PORT_OBJ} ${EXTRA_DEPS}
	$(call libc-archive,${PORT_OBJ})

${CURDIR}/crt1.o: ${PORT_CRT_OBJ}
	cp $< $@

${INSTALL_ROOT}/lib/librtos-none.a: ${CURDIR}/librtos-none.a
	$(call libc-install,$<,$@)

${INSTALL_ROOT}/lib/none/crt1.o: ${CURDIR}/crt1.o
	$(call libc-install,$<,$@)

endif
