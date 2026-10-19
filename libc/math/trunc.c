/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * trunc.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.9.9: clear the fraction bits; no inexact (F.10.6.9) */
double trunc(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits) - __LIBC_DOUBLE_BIAS;

	if (exponent >= 52)
		return exponent == 1024 ? x + x : x;
	if (exponent < 0)
		return __libc_double_from(bits & __LIBC_DOUBLE_SIGN);

	return __libc_double_from(bits & ~(__LIBC_DOUBLE_FRACTION >> exponent));
}
