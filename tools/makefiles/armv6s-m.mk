#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# armv6s-m.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Armv6S-M (Cortex-M0, M0+, M1 with the OS extension): Armv6-M with SVC, no FPU (PLAN D37)

CROSS_FLAGS ?= -mthumb -march=armv6s-m -mfloat-abi=soft

export ARCH_FAMILY := arm
export LIBC_FPU := nofp
# <fenv.h> implementation: x86_64, fpv5-dp (every floating type in the FPU), or soft (D44)
export LIBC_FENV := soft
# libm functions in FPU instructions: libc/math/<layer> replaces the generic files of the same name (D46)
export LIBC_MATH_LAYERS :=

include ${PROJECT_ROOT}/tools/makefiles/arm-none-eabi.mk
