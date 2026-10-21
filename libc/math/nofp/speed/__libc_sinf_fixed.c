/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_sinf_fixed.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdbit.h>
#include <stdint.h>

#include "fixed.h"
#include "libm-fixed.h"

/* sin and cos of j pi/32 in units of 2^-62, j = 0 .. 8 */
static const uint64_t table[9][2] = {
	{ 0x0000000000000000, 0x4000000000000000 },
	{ 0x0645e9af0a6d0af8, 0x3fb11b47a24a4b3c },
	{ 0x0c7c5c1e34d3055b, 0x3ec52f9feeb96056 },
	{ 0x1294062ed59f05a9, 0x3d3e82ad8c5bb4bb },
	{ 0x187de2a6aea962d2, 0x3b20d79e651a8c51 },
	{ 0x1e2b5d3806f63b1e, 0x387165e3017b61a4 },
	{ 0x238e76735cd190d9, 0x3536cc521d434606 },
	{ 0x2899e64a123bac30, 0x317900d62a2e816a },
	{ 0x2d413cccfe779921, 0x2d413cccfe779921 },
};

/* A float in units of 2^-48, rounded toward zero */
static int64_t fixed48(float x)
{
	uint32_t bits;
	int shift;
	int64_t value = 0;

	__builtin_memcpy(&bits, &x, sizeof(bits));
	shift = (int)((bits >> 23) & 0xff) - 102;
	if (shift >= 0)
		value = (int64_t)((bits & 0x007fffff) | 0x00800000) << shift;
	else if (shift > -24 && (bits & 0x7f800000) != 0)
		value = ((bits & 0x007fffff) | 0x00800000) >> -shift;

	return (bits >> 31) != 0 ? -value : value;
}

/*
 * sin(k pi/32 + s) for any k (taken modulo 64) and s = high + low,
 * |s| <= pi/64, from the quadrant and the table angle a = |j| pi/32 as in
 * the float-float core (PLAN D60): first (1 + (cos s - 1)) + second sin s,
 * with first and second the signed sin a or cos a. s is a signed number in
 * units of 2^-48 (exact for |s| >= 2^-24); cos s - 1 = u (-1/2 + u/24 -
 * u^2/720) and sin s = s (1 + u (-1/6 + u/120)) with u = s^2 in units of
 * 2^-38; the sum is formed in units of 2^-62. The result is v
 * 2^(*exponent - 63) with bit 63 of v set; 0 when |s| < 2^-14 with
 * a = 0 in a sine quadrant, where the result needs more of s's bits. The
 * error of the top 32 bits after the leading one, measured over every
 * float, is below __LIBC_FIXED_SIN_ERROR units.
 */
uint64_t __libc_sinf_fixed(int k, float high, float low, int *exponent, bool *negative)
{
	int quadrant = ((k + 8) & 63) / 16;
	int j = ((k + 8) & 15) - 8;
	int a = j < 0 ? -j : j;
	int64_t s = fixed48(high) + fixed48(low);
	uint64_t magnitude = s < 0 ? -(uint64_t)s : (uint64_t)s;
	int32_t s35 = (int32_t)(s >> 13);
	int32_t u = __libc_fixed_high_signed(s35, s35);
	int32_t h = 89478485 - (int32_t)(__libc_fixed_high((uint32_t)u, 2982616) >> 6);
	int32_t cosine_m1;
	int32_t g;
	int64_t first;
	uint64_t second;
	uint64_t product;
	int64_t value;
	bool second_negative;
	int shift;

	h = -(1 << 30) + (__libc_fixed_high_signed(u, h) >> 6);
	cosine_m1 = __libc_fixed_high_signed(u, h);
	g = -357913941 + (int32_t)(__libc_fixed_high((uint32_t)u, 17895697) >> 6);
	g = __libc_fixed_high_signed(u, g);

	if ((quadrant & 1) == 0) {
		/* sin(a sign + s) = sign sin a (1 + cm) + cos a sin s */
		first = (int64_t)table[a][0];
		second = table[a][1];
		if (j < 0)
			first = -first;
		second_negative = s < 0;
		if (a == 0 && magnitude < (UINT64_C(1) << 34))
			return 0;
	} else {
		/* cos(a sign + s) = cos a (1 + cm) - sign sin a sin s */
		first = (int64_t)table[a][1];
		second = table[a][0];
		second_negative = (j < 0) == (s < 0);
	}

	/* |second s| in units of 2^-62 from the 32-bit halves of second (below 2^62) and |s| (below 2^44) */
	{
		uint32_t sh = (uint32_t)(second >> 32);
		uint32_t sl = (uint32_t)second;
		uint32_t th = (uint32_t)(magnitude >> 32);
		uint32_t tl = (uint32_t)magnitude;

		product = (__libc_fixed_wide(sh, th) << 16) + (__libc_fixed_wide(sh, tl) >> 16) + (__libc_fixed_wide(sl, th) >> 16) + (__libc_fixed_wide(sl, tl) >> 48);
	}

	/* first (1 + cm) + second s (1 + g), with cm and g in units of 2^-37 */
	value = (int64_t)product + (__libc_fixed_wide_signed((int32_t)(product >> 30), g) >> 7);
	if (second_negative)
		value = -value;
	value += first + (__libc_fixed_wide_signed((int32_t)(first >> 32), cosine_m1) >> 5);

	*negative = (value < 0) != (quadrant >= 2);
	magnitude = value < 0 ? -(uint64_t)value : (uint64_t)value;
	shift = (int)stdc_leading_zeros(magnitude);
	*exponent = 1 - shift;
	return magnitude << shift;
}
