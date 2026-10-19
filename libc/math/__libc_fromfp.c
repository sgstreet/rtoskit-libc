/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_fromfp.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.10, 7.12.9.11: round x in the math rounding direction rnd
 * (an unknown one rounds to nearest even), then require the integer to fit a
 * signed or unsigned integer of width bits. Otherwise, or for a zero width,
 * an infinity or a NaN, a domain error gives a NaN. The x versions raise
 * inexact when the value changed.
 */
double __libc_fromfp(double x, int rnd, unsigned int width, bool is_unsigned, bool exact)
{
	unsigned int magnitude_bits = is_unsigned ? width : width - 1;
	double rounded;

	if (width == 0 || !isfinite(x))
		return __libc_math_invalid();

	switch (rnd) {
		case FP_INT_UPWARD:
			rounded = ceil(x);
			break;
		case FP_INT_DOWNWARD:
			rounded = floor(x);
			break;
		case FP_INT_TOWARDZERO:
			rounded = trunc(x);
			break;
		case FP_INT_TONEARESTFROMZERO:
			rounded = round(x);
			break;
		default:
			rounded = roundeven(x);
			break;
	}

	/* Every finite double is below 2^1024; below that the limit is exact */
	if (magnitude_bits < 1024) {
		double limit = scalbn(1.0, (int)magnitude_bits);

		if (rounded >= limit || rounded < (is_unsigned ? 0.0 : -limit))
			return __libc_math_invalid();
	} else if (is_unsigned && rounded < 0) {
		return __libc_math_invalid();
	}

	if (exact && rounded != x)
		__libc_math_raise_inexact();

	return rounded;
}
