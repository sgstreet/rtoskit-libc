/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * sqrtf.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"
#include "softfloat.h"

/*
 * C23 7.12.7.10, correctly rounded to nearest, the only direction without
 * an FPU (D44, PLAN D85): x = m 2^(e - 150) with a 24-bit m; with a = m 4
 * (e even) or m 2 (e odd), the root of a 2^24 has 25 bits, a significand
 * and a guard bit, found two radicand bits at a time with the remainder
 * below 2^28; the remainder is the sticky bit. Ties cannot occur.
 */
float sqrtf(float x)
{
	uint32_t bits = __libc_softfloat_bits(x);
	uint32_t root = 0;
	uint32_t remainder = 0;
	uint32_t radicand;
	int exponent;

	if ((bits << 1) == 0 || bits == __LIBC_SOFTFLOAT_INFINITY)
		return x;
	if ((bits << 1) > (__LIBC_SOFTFLOAT_INFINITY << 1))
		return __libc_softfloat_from(bits | __LIBC_SOFTFLOAT_QUIET);
	if ((bits & __LIBC_SOFTFLOAT_SIGN) != 0)
		return __libc_math_invalidf();

	radicand = __libc_softfloat_unpack(bits, &exponent);
	radicand <<= (exponent & 1) != 0 ? 7 : 8;

	/* The 26 bits of a, then zeros: 25 digits of the root */
	for (int i = 0; i < 25; ++i) {
		uint32_t trial;

		remainder = (remainder << 2) | (radicand >> 30);
		radicand <<= 2;
		trial = (root << 2) | 1;
		root <<= 1;
		if (remainder >= trial) {
			remainder -= trial;
			root |= 1;
		}
	}

	/* exponent + 127 > 0 for every normalised operand */
	return __libc_softfloat_from(__libc_softfloat_pack(0, (exponent + 127) >> 1, (root << 1) | (remainder != 0)));
}
