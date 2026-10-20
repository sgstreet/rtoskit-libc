#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# nofloat.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# libc-nofloat.a: the floating-point conversions of the printf and scanf families fail with EINVAL.
# These objects replace members of libc.a in a flavour archive (libc/libc.mk, PLAN D13, D53); they are never in libc.a itself.

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -I${PROJECT_ROOT}/libc
CFLAGS += ${LIBC_CFLAGS}

endif
