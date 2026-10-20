/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_rem_pio2f.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* The first 256 bits of 2/pi: enough for every float (the largest needs bits 103 .. 230) */
static const uint32_t two_over_pi[8] = { 0xa2f9836e, 0x4e441529, 0xfc2757d1, 0xf534ddc0, 0xdb629599, 0x3c439041, 0xfe5163ab, 0xdebbc561 };

/* 32 bits of the 160-bit little-endian number from bit position on (zeros outside) */
static uint32_t bits32(const uint32_t limbs[5], int position)
{
	int word;
	int shift;
	uint32_t value;

	if (position <= -32 || position >= 160)
		return 0;
	if (position < 0)
		return limbs[0] << -position;

	word = position / 32;
	shift = position % 32;
	value = limbs[word] >> shift;
	if (shift != 0 && word + 1 < 5)
		value |= limbs[word + 1] << (32 - shift);

	return value;
}

/*
 * x = (k + d) pi/32 with the integer k taken modulo 64 and |d| <= 1/2, for
 * |x| at least 2^-6: Payne and Hanek's reduction in integers. With
 * x = m 2^e, x 32/pi = m 2^(e+4) (2/pi); the bits of 2/pi before position
 * e - 1 only add multiples of 64, so a 128-bit window from there times the
 * 24-bit m (five 32-bit limbs) gives k and d exactly but for a truncation
 * below 2^-98 of a unit; d keeps about 48 significant bits even when x is
 * very close to a multiple of pi/32. *s = d pi/32 as float-float (PLAN D60).
 */
int __libc_rem_pio2f(float x, struct __libc_ff *s)
{
	uint32_t bits = __libc_float_bits(x);
	int e = __libc_float_exponent(bits) - __LIBC_FLOAT_BIAS - 23;
	uint32_t m = (bits & __LIBC_FLOAT_FRACTION) | (1U << 23);
	int first = e - 1 > 1 ? e - 1 : 1;
	int word = (first - 1) / 32;
	int offset = (first - 1) % 32;
	int shift = first + 123 - e;
	uint32_t window[4];
	uint32_t limbs[5];
	uint64_t carry = 0;
	bool upper;
	int k;
	int top;

	/* The 128 bits of 2/pi from position first, least significant word first */
	for (int i = 0; i < 4; ++i) {
		uint32_t a = two_over_pi[word + 3 - i];
		uint32_t b = word + 4 - i < 8 ? two_over_pi[word + 4 - i] : 0;

		window[i] = offset != 0 ? (a << offset) | (b >> (32 - offset)) : a;
	}
	for (int i = 0; i < 4; ++i) {
		uint64_t product = (uint64_t)m * window[i] + carry;

		limbs[i] = (uint32_t)product;
		carry = product >> 32;
	}
	limbs[4] = (uint32_t)carry;

	/* |x| 32/pi = P 2^-shift: k is the integer part modulo 64; a fraction of 1/2 or more counts as k + 1 and d - 1 */
	k = (int)(bits32(limbs, shift) & 63);
	upper = (bits32(limbs, shift - 1) & 1) != 0;
	if (upper) {
		uint64_t borrow = 0;

		++k;
		for (int i = 0; i < 5; ++i) {
			uint64_t difference = (uint64_t)0 - limbs[i] - borrow;

			limbs[i] = (uint32_t)difference;
			borrow = (difference >> 32) != 0;
		}
	}
	for (int i = 0; i < 5; ++i) {
		if (32 * i >= shift)
			limbs[i] = 0;
		else if (32 * (i + 1) > shift)
			limbs[i] &= (1U << (shift - 32 * i)) - 1;
	}

	/* The leading 48 bits of |d| as float-float, times pi/32 */
	for (top = 4; top >= 0 && limbs[top] == 0; --top);
	if (top < 0) {
		*s = (struct __libc_ff){ .hi = 0.0f, .lo = 0.0f };
	} else {
		float high;
		float low;

		top = 32 * top + 31 - __builtin_clz(limbs[top]);
		high = scalbnf((float)(bits32(limbs, top - 23) & 0xffffff), top - 23 - shift);
		low = scalbnf((float)(bits32(limbs, top - 47) & 0xffffff), top - 47 - shift);
		*s = __libc_ff_multiply(__libc_ff_fast_sum(high, low), __LIBC_PI_32F);
	}

	/* The sign of d, then the sign of x: x = -(k + d) pi/32 for a negative x */
	if (upper != ((bits & __LIBC_FLOAT_SIGN) != 0))
		*s = (struct __libc_ff){ .hi = -s->hi, .lo = -s->lo };
	if ((bits & __LIBC_FLOAT_SIGN) != 0)
		k = -k;

	return k & 63;
}
