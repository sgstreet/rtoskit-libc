/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * logbf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.6.17: the unbiased exponent as a float; logbf(+-0) is -infinity with divide-by-zero (F.10.3.17) */
float logbf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits);

	if (exponent == 0xff)
		return x * x;
	if (exponent == 0) {
		if ((bits << 1) == 0)
			return -1.0f / fabsf(x);
		exponent = __libc_float_exponent(__libc_float_bits(x * 0x1p32f)) - 32;
	}

	return (float)(exponent - __LIBC_FLOAT_BIAS);
}
