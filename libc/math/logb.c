/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * logb.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.6.17, F.10.3.17: the unbiased exponent as a double; logb(+-0) is
 * -infinity with divide-by-zero, logb(+-infinity) is +infinity.
 */
double logb(double x)
{
	uint64_t bits = __libc_double_bits(x);
	int exponent = __libc_double_exponent(bits);

	if (exponent == 0x7ff)
		return x * x;
	if (exponent == 0) {
		if ((bits << 1) == 0)
			return -1.0 / fabs(x);
		exponent = __libc_double_exponent(__libc_double_bits(x * 0x1p64)) - 64;
	}

	return exponent - __LIBC_DOUBLE_BIAS;
}
