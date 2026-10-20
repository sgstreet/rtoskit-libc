#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# sp-nearest.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

# Float-float is exact only when rounding to nearest: the layer serves the cores whose <fenv.h> has no other direction (PLAN D57)
$(if $(filter soft,${LIBC_FENV}),,$(error libc/math/sp-nearest needs LIBC_FENV=soft, not '${LIBC_FENV}'))

EXTRA_TARGETS := ${CURDIR}/sp-nearest.checked

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -I${PROJECT_ROOT}/libc
CFLAGS += ${LIBC_CFLAGS} -fno-math-errno

# The layer computes in float only, as libc/math/sp (PLAN D56)
CFLAGS += -Wdouble-promotion
SP_FORBIDDEN := __aeabi_(d[a-z0-9]+|f2d|f2u?lz|u?l2[df]|u?i2d)
${CURDIR}/sp-nearest.checked: ${OBJ}
	@echo "CHECKING ${@}"
	@if ${NM} ${OBJ} | grep -E ' U ${SP_FORBIDDEN}$$'; then echo "libc/math/sp-nearest: double-precision helper referenced" >&2; exit 1; fi
	@touch $@

endif
