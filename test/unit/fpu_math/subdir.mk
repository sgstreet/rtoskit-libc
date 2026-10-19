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

# Only cores with FPU layers (D46) have instruction versions to compare with the generic C ones
ifneq (${LIBC_MATH_LAYERS},)
SRC := ${SOURCE_DIR}/fpu_math.c ${SOURCE_DIR}/generic.c
endif
