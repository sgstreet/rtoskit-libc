#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# armv6-m.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Armv6-M without the OS extension (no SVC instruction), no FPU: the baseline Thumb-1 architecture (PLAN D37)

CROSS_FLAGS ?= -mthumb -march=armv6-m -mfloat-abi=soft

export ARCH_FAMILY := arm
export LIBC_FPU := nofp

include ${PROJECT_ROOT}/tools/makefiles/arm-none-eabi.mk
