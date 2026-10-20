/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_fromfpf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.9.10, 7.12.9.11, in float: round x in the math rounding
 * direction rnd (an unknown one rounds to nearest even), then require the
 * integer to fit a signed or unsigned integer of width bits. Otherwise, or
 * for a zero width, an infinity or a NaN, a domain error gives a NaN. The x
 * versions raise inexact when the value changed.
 */
float __libc_fromfpf(float x, int rnd, unsigned int width, bool is_unsigned, bool exact)
{
	unsigned int magnitude_bits = is_unsigned ? width : width - 1;
	float rounded;

	if (width == 0 || !isfinite(x))
		return __libc_math_invalidf();

	switch (rnd) {
		case FP_INT_UPWARD:
			rounded = ceilf(x);
			break;
		case FP_INT_DOWNWARD:
			rounded = floorf(x);
			break;
		case FP_INT_TOWARDZERO:
			rounded = truncf(x);
			break;
		case FP_INT_TONEARESTFROMZERO:
			rounded = roundf(x);
			break;
		default:
			rounded = roundevenf(x);
			break;
	}

	/* Every finite float is below 2^128; below that the limit is exact */
	if (magnitude_bits < 128) {
		float limit = scalbnf(1.0f, (int)magnitude_bits);

		if (rounded >= limit || rounded < (is_unsigned ? 0.0f : -limit))
			return __libc_math_invalidf();
	} else if (is_unsigned && rounded < 0) {
		return __libc_math_invalidf();
	}

	if (exact && rounded != x)
		__libc_math_raise_inexact();

	return rounded;
}
