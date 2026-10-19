#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# armv7e-m-dp.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Armv7E-M with the double-precision FPv5-D16 (Cortex-M7 DP), hard-float ABI (PLAN D18)

CROSS_FLAGS ?= -mthumb -march=armv7e-m+fp.dp -mfloat-abi=hard

export ARCH_FAMILY := arm
export LIBC_FPU := fpv5-dp

include ${PROJECT_ROOT}/tools/makefiles/arm-none-eabi.mk
