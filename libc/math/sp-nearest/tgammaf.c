/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tgammaf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.8.4: +-e^lgamma(x) through the exp2 core, the sign from the
 * lgamma core; tgamma(+-0) is +-infinity with divide-by-zero, a negative
 * integer or -infinity is invalid; beyond 35.1 the result overflows and
 * below -50 it is +-0 with the sign of the interval (PLAN D64).
 */
float tgammaf(float x)
{
	struct __libc_ff value;
	int sign;

	if (isnan(x) || x == INFINITY)
		return x + x;
	if (x == 0)
		return 1.0f / x;
	if (x < 0 && truncf(x) == x)
		return __libc_math_invalidf();
	if (x > 35.1f)
		return 0x1p127f * 0x1p127f;
	if (x < -50.0f) {
		/* gamma is negative on (-2k - 1, -2k) */
		sign = ((int32_t)floorf(x) & 1) != 0 ? -1 : 1;
		return (float)sign * 0x1p-100f * 0x1p-100f;
	}

	value = __libc_lgammaf_core(x, &sign);

	return __libc_exp2f_signed(__libc_ff_multiply(value, __LIBC_INV_LN2F), sign < 0);
}
