#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Only the double-precision FPv5-D16 has every instruction the test runs (PLAN D18)
ifeq (${LIBC_FPU},fpv5-dp)
SRC += ${SOURCE_DIR}/fpv5.c
endif
