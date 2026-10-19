#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# fenv.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
SUBDIRS := ${LIBC_FENV}
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

include ${PROJECT_ROOT}/tools/makefiles/project.mk

# The generic functions use the ${LIBC_FENV} variant's register access (fenv-arch.h)
CPPFLAGS += -I${PROJECT_ROOT}/libc -I${SOURCE_DIR}/${LIBC_FENV}
CFLAGS += ${LIBC_CFLAGS}

endif
