/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_softdouble_pack.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "softdouble.h"

/*
 * The double nearest x 2^(exponent - 1085), ties to even, with the sign:
 * x lies in [2^62, 2^63), the significand above ten more bits, of which
 * anything nonzero below the first must have reached x (a sticky bit). A
 * result below the normal range is shifted first, so it is rounded only
 * once, on the subnormal grid; a carry out of the significand moves to the
 * next exponent through the addition, to infinity at the top.
 */
uint64_t __libc_softdouble_pack(uint64_t sign, int exponent, uint64_t x)
{
	uint64_t significand;
	uint32_t rest;
	uint64_t bits;

	if (exponent >= 2047)
		return sign | __LIBC_SOFTDOUBLE_INFINITY;
	if (exponent <= 0) {
		unsigned int shift = (unsigned int)(1 - exponent);

		x = shift >= 63 ? x != 0 : (x >> shift) | ((x & ((1ULL << shift) - 1)) != 0);
		exponent = 1;
	}

	significand = x >> 10;
	rest = (uint32_t)x & 0x3ff;
	if (rest > 0x200 || (rest == 0x200 && (significand & 1) != 0))
		++significand;

	bits = ((uint64_t)(exponent - 1) << 52) + significand;
	if (bits >= __LIBC_SOFTDOUBLE_INFINITY)
		return sign | __LIBC_SOFTDOUBLE_INFINITY;

	return sign | bits;
}
