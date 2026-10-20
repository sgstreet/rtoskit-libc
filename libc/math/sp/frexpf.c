/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * frexpf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.7: x = f * 2^*exp with |f| in [1/2, 1), from the bits; a subnormal is scaled up exactly first */
float frexpf(float x, int *exp)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits);

	if (exponent == 0xff) {
		*exp = 0;
		return x + x;
	}
	if (exponent == 0) {
		if ((bits << 1) == 0) {
			*exp = 0;
			return x;
		}
		bits = __libc_float_bits(x * 0x1p32f);
		exponent = __libc_float_exponent(bits) - 32;
	}

	*exp = exponent - (__LIBC_FLOAT_BIAS - 1);
	return __libc_float_from((bits & ~__LIBC_FLOAT_EXPONENT) | ((uint32_t)(__LIBC_FLOAT_BIAS - 1) << 23));
}
