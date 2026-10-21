/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp2f_fixed.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "fixed.h"
#include "libm-fixed.h"

/* 2^(j/32) - 1 in units of 2^-32, rounded to nearest, j = 0 .. 31 */
static const uint32_t table[32] = {
	0x00000000,
	0x059b0d31,
	0x0b5586d0,
	0x11301d01,
	0x172b83c8,
	0x1d487317,
	0x2387a6e7,
	0x29e9df52,
	0x306fe0a3,
	0x371a7374,
	0x3dea64c1,
	0x44e08606,
	0x4bfdad53,
	0x5342b56a,
	0x5ab07dd5,
	0x6247eb04,
	0x6a09e668,
	0x71f75e8f,
	0x7a11473f,
	0x82589995,
	0x8ace5423,
	0x93737b0d,
	0x9c49182a,
	0xa5503b24,
	0xae89f996,
	0xb7f76f30,
	0xc199bdd8,
	0xcb720dcf,
	0xd5818dd0,
	0xdfc97338,
	0xea4afa2a,
	0xf50765b7,
};

/*
 * 2^z = 2^n 2^(j/32) 2^r for z = Z 2^-37, n = floor(z), j the next five
 * bits and r = (Z mod 2^32) 2^-37 in [0, 2^-5). 2^r - 1 is the Taylor
 * polynomial of degree 4 in r ln 2 (the next term is below 2^-34), by
 * Horner's rule with every coefficient scaled to fill 32 bits: (ln 2)^k / k!
 * in units of 2^-32, 2^-34, 2^-36 and 2^-38, and r times a coefficient in
 * the units of the next one after a shift by 7. The result is 2^z as
 * 2^n (1 + F 2^-32); the error of F, measured over every float argument,
 * is below __LIBC_FIXED_EXP2_ERROR units (docs/research/libm-fixed.md).
 */
uint32_t __libc_exp2f_fixed(int64_t z, int *exponent)
{
	uint32_t r = (uint32_t)z;
	uint32_t t = table[(uint32_t)(z >> 32) & 31];
	uint32_t a = 0xe35846b8 + (__libc_fixed_high(r, 0x9d955b7e) >> 7);
	uint32_t f;

	a = 0xf5fdeffc + (__libc_fixed_high(r, a) >> 7);
	a = 0xb17217f8 + (__libc_fixed_high(r, a) >> 7);

	/* 2^r - 1 = p 2^-37, below 2^-5.4 */
	a = __libc_fixed_high(r, a);

	/* (1 + t)(1 + p) - 1, saturated below 2: the true value is below 2 and the rounding then goes up as it must */
	f = t + (a >> 5) + (__libc_fixed_high(t, a) >> 5);
	if (f < t)
		f = UINT32_MAX;

	*exponent = (int)(z >> 37);
	return f;
}
