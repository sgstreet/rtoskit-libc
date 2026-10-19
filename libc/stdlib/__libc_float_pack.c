/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_float_pack.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdbit.h>
#include <stdint.h>

#include "internal/strtofloat.h"

/*
 * C23 7.24.1.5p10 and F.5: an overflowing value gives infinity, or the
 * largest finite value when the rounding direction points toward zero.
 */
static uint64_t overflow(bool negative, const struct __libc_float_format *format, int rounding)
{
	uint64_t infinity = __libc_float_infinity(format);
	bool to_infinity = rounding == __LIBC_ROUND_NEAREST || (rounding == __LIBC_ROUND_UPWARD && !negative) || (rounding == __LIBC_ROUND_DOWNWARD && negative);

	errno = ERANGE;
	return (negative ? __libc_float_sign(format) : 0) | (to_infinity ? infinity : infinity - 1);
}

uint64_t __libc_float_pack(bool negative, uint64_t significand, int exponent, bool sticky, const struct __libc_float_format *format, int rounding)
{
	unsigned int precision = format->precision;
	uint64_t sign = negative ? __libc_float_sign(format) : 0;
	uint64_t mantissa;
	uint64_t bits;
	unsigned int shift;
	bool round;
	bool increment;

	if (significand == 0)
		return sign;

	/* Normalise to the top bit; the value lies in [2^top, 2^(top + 1)) */
	unsigned int leading = stdc_leading_zeros(significand);

	significand <<= leading;
	exponent -= (int)leading;
	int top = exponent + 63;

	if (top > format->emax)
		return overflow(negative, format, rounding);

	/* Keep precision bits; a subnormal result keeps fewer, so it is rounded only once */
	bool tiny = top < format->emin;
	int drop = 64 - (int)precision + (tiny ? format->emin - top : 0);

	if (drop > 64) {
		mantissa = 0;
		round = false;
		sticky = true;
	} else if (drop == 64) {
		mantissa = 0;
		round = significand >> 63;
		sticky = sticky || (significand << 1) != 0;
	} else {
		shift = (unsigned int)drop;
		mantissa = significand >> shift;
		round = (significand >> (shift - 1)) & 1;
		sticky = sticky || (significand & (((uint64_t)1 << (shift - 1)) - 1)) != 0;
	}

	switch (rounding) {
		case __LIBC_ROUND_TOWARD_ZERO:
			increment = false;
			break;
		case __LIBC_ROUND_UPWARD:
			increment = !negative && (round || sticky);
			break;
		case __LIBC_ROUND_DOWNWARD:
			increment = negative && (round || sticky);
			break;
		default:
			increment = round && (sticky || (mantissa & 1) != 0);
			break;
	}
	mantissa += increment;

	/*
	 * A normal mantissa carries its implicit bit into the exponent field, and
	 * a carry out of it moves to the next binade; a subnormal that rounds up
	 * to 2^(precision - 1) becomes the smallest normal number the same way.
	 */
	bits = tiny ? mantissa : ((uint64_t)(top - format->emin) << (precision - 1)) + mantissa;
	if (bits >= __libc_float_infinity(format))
		return overflow(negative, format, rounding);

	/* C23 7.12.1p6: underflow is a tiny and inexact result */
	if (tiny && (round || sticky))
		errno = ERANGE;

	return sign | bits;
}
