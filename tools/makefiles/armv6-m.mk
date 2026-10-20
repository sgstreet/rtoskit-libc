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
# <fenv.h> implementation: x86_64, fpv5-dp (every floating type in the FPU), or soft (D44)
export LIBC_FENV := soft
# libm layers: libc/math/<layer> replaces the generic files of the same name and may add helpers (D46, D55)
export LIBC_MATH_LAYERS := sp sp-nearest nofp
# Hand-written string kernels (libc/string/<kernels>), PLAN D69
export LIBC_STRING_KERNELS := thumb1

include ${PROJECT_ROOT}/tools/makefiles/arm-none-eabi.mk
