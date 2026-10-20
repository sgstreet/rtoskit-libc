#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# harness.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# The harness objects (no archive): the generic part and the ${ARCH_FAMILY} cycle counter

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
SUBDIRS := ${ARCH_FAMILY}
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/harness.mk

include ${PROJECT_ROOT}/tools/makefiles/project.mk

endif
