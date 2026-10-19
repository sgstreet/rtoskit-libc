/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exact_to_double.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>
#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* Whether rounding the kept bits away from zero is due, in the current direction (FLT_ROUNDS) */
static bool round_away(bool negative, uint64_t kept, bool half, bool below)
{
	switch (FLT_ROUNDS) {
		case 0:
			return false;
		case 2:
			return !negative && (half || below);
		case 3:
			return negative && (half || below);
		default:
			return half && (below || (kept & 1) != 0);
	}
}

/*
 * C23 F.3, F.7: round an exact value to double once. A normal result keeps
 * DBL_MANT_DIG bits and lets one addition do the rounding (the bits below are
 * coded as a quarter, half or three quarters of a unit), then scales exactly
 * (or overflows) with scalbn. A subnormal result is rounded here from
 * the direction FLT_ROUNDS reports, raising underflow (tiny before rounding)
 * when inexact.
 */
double __libc_exact_to_double(const struct __libc_exact *value)
{
	uint64_t high = value->high;
	uint64_t low = value->low;
	int exponent = value->exponent;
	int top;
	int precision;
	uint64_t kept;
	bool half;
	bool below;

	if (high == 0) {
		high = low;
		low = 0;
		exponent -= 64;
	}
	if (__builtin_clzll(high) != 0) {
		int shift = __builtin_clzll(high);

		high = (high << shift) | (low >> (64 - shift));
		low <<= shift;
		exponent -= shift;
	}

	/* The leading bit's exponent, and how many bits fit at that magnitude */
	top = exponent + 127;
	precision = top >= (DBL_MIN_EXP - 1) ? DBL_MANT_DIG : top - (DBL_MIN_EXP - 1) + DBL_MANT_DIG;

	if (precision >= 1) {
		uint64_t rest = high << precision;

		kept = high >> (64 - precision);
		half = (rest >> 63) != 0;
		below = (rest << 1) != 0 || low != 0 || value->sticky;
	} else {
		kept = 0;
		half = precision == 0;
		below = precision < 0 || (high << 1) != 0 || low != 0 || value->sticky;
	}

	if (top >= (DBL_MIN_EXP - 1)) {
		double tail = half ? (below ? 0.75 : 0.5) : (below ? 0.25 : 0.0);
		double rounded = value->negative ? -(double)kept - tail : (double)kept + tail;

		return scalbn(rounded, top - (DBL_MANT_DIG - 1));
	}

	if (round_away(value->negative, kept, half, below))
		++kept;
	if (half || below)
		__libc_math_raise_underflow();

	return __libc_double_from((value->negative ? __LIBC_DOUBLE_SIGN : 0) | (uint64_t)kept);
}
