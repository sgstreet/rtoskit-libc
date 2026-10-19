/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * decimal.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_DECIMAL_H
#define _INTERNAL_DECIMAL_H

#include <stdint.h>

/*
 * Exact decimal arithmetic for the conversions between binary floating
 * point and text (strtod, printf; docs/research/strtod.md and
 * docs/research/printf-float.md). The value is
 * 0.digits[0]digits[1]...digits[count - 1] * 10^point, with digits[0]
 * non-zero (count 0 is zero), plus "truncated" when non-zero digits beyond
 * the kept ones were dropped.
 */

/* Digits kept; a binary64 value or rounding boundary has at most 769 significant digits */
#define __LIBC_DECIMAL_DIGITS 800

/*
 * The largest shift one pass handles: a digit times 2^28 plus the carry stays
 * below 10 * 2^28 < 2^32, so the accumulators are 32 bits wide and dividing
 * by ten needs no 64-bit division (a library call on the 32-bit targets).
 */
#define __LIBC_DECIMAL_MAX_SHIFT 28

/* Digits a left shift by the maximum adds at most (2^28 has 9 digits) */
#define __LIBC_DECIMAL_SHIFT_DIGITS 9

/* Rounding directions, as the values of FLT_ROUNDS (C23 5.2.5.3.3) */
#define __LIBC_ROUND_TOWARD_ZERO 0
#define __LIBC_ROUND_NEAREST 1
#define __LIBC_ROUND_UPWARD 2
#define __LIBC_ROUND_DOWNWARD 3

struct __libc_decimal {
	int count;
	int point;
	bool truncated;
	unsigned char digits[__LIBC_DECIMAL_DIGITS + __LIBC_DECIMAL_SHIFT_DIGITS + 1];
};

/* Drop trailing zeros, which do not change the value */
static inline void __libc_decimal_trim(struct __libc_decimal *d)
{
	while (d->count > 0 && d->digits[d->count - 1] == 0)
		--d->count;
}

/* Multiply or divide by 2^shift, 1 <= shift <= __LIBC_DECIMAL_MAX_SHIFT */
void __libc_decimal_shift_left(struct __libc_decimal *d, unsigned int shift);
void __libc_decimal_shift_right(struct __libc_decimal *d, unsigned int shift);

/* Set d to significand * 2^exponent; exact for every binary64 value */
void __libc_decimal_set(struct __libc_decimal *d, uint64_t significand, int exponent);

/*
 * Keep the first keep digits (counted from digits[0], so keep <= 0 rounds at
 * or above the leading digit's position), rounding the magnitude of a
 * value of the given sign in the given direction with the exact tail.
 */
void __libc_decimal_round(struct __libc_decimal *d, int keep, bool negative, int rounding);

#endif /* _INTERNAL_DECIMAL_H */
