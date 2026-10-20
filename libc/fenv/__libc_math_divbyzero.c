/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_math_divbyzero.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "fenv-arch.h"
#include "internal/math.h"

/* sign / 0: an infinity and divide-by-zero; a constant where no exception is recorded (PLAN D55) */
double __libc_math_divbyzero(double sign)
{
	if (__libc_fenv_records) {
		volatile double zero = 0.0;

		return sign / zero;
	}

	return __builtin_copysign(__builtin_inf(), sign);
}
