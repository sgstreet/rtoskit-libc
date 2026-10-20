/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * truncf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.8: the fraction bits cleared (no inexact, F.10.6.8) */
float truncf(float x)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits) - __LIBC_FLOAT_BIAS;

	if (exponent >= 23)
		return exponent == 128 ? x + x : x;
	if (exponent < 0)
		return __libc_float_from(bits & __LIBC_FLOAT_SIGN);

	return __libc_float_from(bits & ~(__LIBC_FLOAT_FRACTION >> exponent));
}
