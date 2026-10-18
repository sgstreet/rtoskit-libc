#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# api.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

LIBC_TEST_SUITE := api
EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a

# Header tests have no main(); link each with the suite's main.c
LIBC_TEST_EXTRA := main

include ${PROJECT_ROOT}/tools/makefiles/project.mk
include ${PROJECT_ROOT}/tools/makefiles/libc-test.mk

endif
