#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# armv8-m.main.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Armv8-M Mainline with DSP and the single-precision FPv5-SP-D16 (Cortex-M33, M35P), hard-float ABI

CROSS_FLAGS ?= -mthumb -march=armv8-m.main+fp+dsp -mfloat-abi=hard

export ARCH_FAMILY := arm
export LIBC_FPU := fpv5-sp

include ${PROJECT_ROOT}/tools/makefiles/arm-none-eabi.mk
