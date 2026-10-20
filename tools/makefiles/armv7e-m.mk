#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# armv7e-m.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Armv7E-M with the single-precision FPv4-SP-D16 (Cortex-M4F, M7 SP), hard-float ABI

CROSS_FLAGS ?= -mthumb -march=armv7e-m+fp -mfloat-abi=hard

export ARCH_FAMILY := arm
export LIBC_FPU := fpv4-sp
# <fenv.h> implementation: x86_64, fpv5-dp (every floating type in the FPU), or soft (D44)
export LIBC_FENV := soft
# libm layers: libc/math/<layer> replaces the generic files of the same name and may add helpers (D46, D55)
export LIBC_MATH_LAYERS := sp fpv4-sp

include ${PROJECT_ROOT}/tools/makefiles/arm-none-eabi.mk
