/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_decimal_to_float.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/decimal.h"
#include "internal/strtofloat.h"

/*
 * Simple Decimal Conversion (docs/research/strtod.md): the exact decimal
 * value is multiplied and divided by powers of two until it lies in
 * [1/2, 1); 64 more bits then give the significand.
 */

/* Beyond these decimal exponents every format overflows, or rounds as a tiny value */
#define MAX_POINT 310
#define MIN_POINT -343

static unsigned int limit_shift(int shift)
{
	return shift < 1 ? 1 : shift > __LIBC_DECIMAL_MAX_SHIFT ? __LIBC_DECIMAL_MAX_SHIFT :
															  (unsigned int)shift;
}

uint64_t __libc_decimal_to_float(const char *begin, const char *end, int exponent10, bool negative, const struct __libc_float_format *format, int rounding)
{
	struct __libc_decimal d = { .count = 0, .point = 0, .truncated = false };
	bool seen_point = false;
	int exponent2 = 0;

	/* Keep the significant digits, skipping leading zeros, and place the decimal point */
	for (const char *s = begin; s < end; ++s) {
		if (*s == '.') {
			seen_point = true;
			continue;
		}

		unsigned char digit = *s - '0';

		if (d.count == 0 && digit == 0) {
			if (seen_point)
				--d.point;
			continue;
		}
		if (!seen_point)
			++d.point;
		if (d.count < __LIBC_DECIMAL_DIGITS)
			d.digits[d.count++] = digit;
		else if (digit != 0)
			d.truncated = true;
	}
	__libc_decimal_trim(&d);

	/* exponent10 is saturated by the parser, so this cannot overflow */
	d.point += exponent10;

	if (d.count == 0)
		return __libc_float_pack(negative, 0, 0, false, format, rounding);
	if (d.point > MAX_POINT)
		return __libc_float_pack(negative, 1, 4 * MAX_POINT, false, format, rounding);
	if (d.point < MIN_POINT)
		return __libc_float_pack(negative, 1, 4 * MIN_POINT, true, format, rounding);

	/* Into [1/2, 1): a shift by 3 * (point - 1) bits keeps the value at least 1, by 3 * -point below 1 */
	while (d.point > 0) {
		unsigned int shift = limit_shift(3 * (d.point - 1));

		__libc_decimal_shift_right(&d, shift);
		exponent2 += shift;
	}
	while (d.point < 0 || d.digits[0] < 5) {
		unsigned int shift = limit_shift(-3 * d.point);

		__libc_decimal_shift_left(&d, shift);
		exponent2 -= shift;
	}

	/* The value times 2^64 lies in [2^63, 2^64): its integer part is the significand */
	for (int shift = 64; shift > 0; shift -= __LIBC_DECIMAL_MAX_SHIFT)
		__libc_decimal_shift_left(&d, shift < __LIBC_DECIMAL_MAX_SHIFT ? shift : __LIBC_DECIMAL_MAX_SHIFT);

	uint64_t significand = 0;

	for (int i = 0; i < d.point; ++i)
		significand = significand * 10 + (i < d.count ? d.digits[i] : 0);

	return __libc_float_pack(negative, significand, exponent2 - 64, d.truncated || d.count > d.point, format, rounding);
}
