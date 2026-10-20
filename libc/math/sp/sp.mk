#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# sp.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_TARGETS := ${CURDIR}/sp.checked

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -I${PROJECT_ROOT}/libc
CFLAGS += ${LIBC_CFLAGS} -frounding-math -fsignaling-nans -fno-math-errno

# The layer computes in float only (PLAN D56): warn on every implicit promotion to double
CFLAGS += -Wdouble-promotion

# ... and refuse any object that still calls a double soft-float helper, or a 64-bit conversion that computes in double
SP_FORBIDDEN := __aeabi_(d[a-z0-9]+|f2d|f2u?lz|u?l2[df]|u?i2d)
${CURDIR}/sp.checked: ${OBJ}
	@echo "CHECKING ${@}"
	@if ${NM} ${OBJ} | grep -E ' U ${SP_FORBIDDEN}$$'; then echo "libc/math/sp: double-precision helper referenced" >&2; exit 1; fi
	@touch $@

endif
