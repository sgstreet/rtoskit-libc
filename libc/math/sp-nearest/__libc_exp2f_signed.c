/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp2f_signed.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * The end of every power function: +-2^z rounded once. Far above 2^128 the
 * result overflows and far below 2^-151 it is zero; in between, the exp2
 * core and its scaling decide (PLAN D59).
 */
float __libc_exp2f_signed(struct __libc_ff z, bool negative)
{
	float sign = negative ? -1.0f : 1.0f;
	int exponent;
	struct __libc_ff value;

	if (z.hi > 128.5f)
		return sign * 0x1p127f * 0x1p127f;
	if (z.hi < -151.5f)
		return sign * 0x1p-100f * 0x1p-100f;

	value = __libc_exp2f_core(z, &exponent);
	if (negative)
		value = (struct __libc_ff){ .hi = -value.hi, .lo = -value.lo };

	return __libc_exp2f_scale(value, exponent);
}
