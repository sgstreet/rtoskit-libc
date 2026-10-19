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
#include <string.h>

#include "internal/strtofloat.h"

/*
 * Simple Decimal Conversion (docs/research/strtod.md): the value is
 * 0.d[0]d[1]...d[count - 1] * 10^point, exactly, plus "truncated" when
 * non-zero digits beyond the kept ones were dropped. Multiplying and
 * dividing it by powers of two brings it into [1/2, 1); 64 more bits then
 * give the significand.
 */

/*
 * The largest shift one pass handles: a digit times 2^28 plus the carry stays
 * below 10 * 2^28 < 2^32, so the accumulators are 32 bits wide and dividing
 * by ten needs no 64-bit division (a library call on the 32-bit targets).
 */
#define MAX_SHIFT 28

/* Digits a left shift by MAX_SHIFT adds at most (2^28 has 9 digits) */
#define SHIFT_DIGITS 9

/* Beyond these decimal exponents every format overflows, or rounds as a tiny value */
#define MAX_POINT 310
#define MIN_POINT -343

struct decimal {
	int count;
	int point;
	bool truncated;
	unsigned char digits[__LIBC_DECIMAL_DIGITS + SHIFT_DIGITS + 1];
};

static void trim(struct decimal *d)
{
	while (d->count > 0 && d->digits[d->count - 1] == 0)
		--d->count;
}

/* Divide by 2^shift: a sliding accumulator reads digits in and writes quotient digits out */
static void shift_right(struct decimal *d, unsigned int shift)
{
	uint32_t mask = ((uint32_t)1 << shift) - 1;
	uint32_t accumulator = 0;
	int read = 0;
	int write = 0;

	/* Read until the first quotient digit is non-zero, past the end with zeros if needed */
	while ((accumulator >> shift) == 0) {
		accumulator = accumulator * 10 + (read < d->count ? d->digits[read] : 0);
		++read;
	}
	d->point -= read - 1;

	/* The write position stays behind the read position */
	while (read < d->count) {
		unsigned char digit = accumulator >> shift;

		accumulator = (accumulator & mask) * 10 + d->digits[read++];
		d->digits[write++] = digit;
	}

	while (accumulator != 0) {
		unsigned char digit = accumulator >> shift;

		accumulator = (accumulator & mask) * 10;
		if (write < __LIBC_DECIMAL_DIGITS)
			d->digits[write++] = digit;
		else if (digit != 0)
			d->truncated = true;
	}

	d->count = write;
	trim(d);
}

/* Multiply by 2^shift: carries run from the last digit, the result is written right-aligned and moved down */
static void shift_left(struct decimal *d, unsigned int shift)
{
	uint32_t accumulator = 0;
	int write = d->count + SHIFT_DIGITS;

	/* One division per digit: the remainder comes from the quotient (-Os does not merge / and %) */
	for (int read = d->count - 1; read >= 0; --read) {
		accumulator += (uint32_t)d->digits[read] << shift;

		uint32_t quotient = accumulator / 10;

		d->digits[--write] = accumulator - quotient * 10;
		accumulator = quotient;
	}
	while (accumulator != 0) {
		uint32_t quotient = accumulator / 10;

		d->digits[--write] = accumulator - quotient * 10;
		accumulator = quotient;
	}

	int count = d->count + SHIFT_DIGITS - write;

	d->point += count - d->count;
	memmove(d->digits, d->digits + write, count);

	if (count > __LIBC_DECIMAL_DIGITS) {
		for (int i = __LIBC_DECIMAL_DIGITS; i < count; ++i)
			d->truncated = d->truncated || d->digits[i] != 0;
		count = __LIBC_DECIMAL_DIGITS;
	}
	d->count = count;
	trim(d);
}

static unsigned int limit_shift(int shift)
{
	return shift < 1 ? 1 : shift > MAX_SHIFT ? MAX_SHIFT :
											   (unsigned int)shift;
}

uint64_t __libc_decimal_to_float(const char *begin, const char *end, int exponent10, bool negative, const struct __libc_float_format *format, int rounding)
{
	struct decimal d = { .count = 0, .point = 0, .truncated = false };
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
	trim(&d);

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

		shift_right(&d, shift);
		exponent2 += shift;
	}
	while (d.point < 0 || d.digits[0] < 5) {
		unsigned int shift = limit_shift(-3 * d.point);

		shift_left(&d, shift);
		exponent2 -= shift;
	}

	/* The value times 2^64 lies in [2^63, 2^64): its integer part is the significand */
	for (int shift = 64; shift > 0; shift -= MAX_SHIFT)
		shift_left(&d, shift < MAX_SHIFT ? shift : MAX_SHIFT);

	uint64_t significand = 0;

	for (int i = 0; i < d.point; ++i)
		significand = significand * 10 + (i < d.count ? d.digits[i] : 0);

	return __libc_float_pack(negative, significand, exponent2 - 64, d.truncated || d.count > d.point, format, rounding);
}
