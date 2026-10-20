/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_math_underflow.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "fenv-arch.h"
#include "internal/math.h"

/* sign * 2^-1022 * 2^-1022: rounded in the current direction with underflow; a signed zero where only rounding to nearest exists (PLAN D55) */
double __libc_math_underflow(double sign)
{
	if (__libc_fenv_records) {
		volatile double tiny = 0x1p-1022;

		return sign * tiny * tiny;
	}

	return __builtin_copysign(0.0, sign);
}
