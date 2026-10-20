/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp2f_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* 2^(j/32) as float-float, j = 0 .. 31 */
static const float table[32][2] = {
	{ 0x1p0f, 0.0f },
	{ 0x1.059b0ep0f, -0x1.9d4f52p-25f },
	{ 0x1.0b5586p0f, 0x1.9f3122p-25f },
	{ 0x1.11301ep0f, -0x1.fdb496p-25f },
	{ 0x1.172b84p0f, -0x1.c15742p-27f },
	{ 0x1.1d4874p0f, -0x1.d2e8cap-25f },
	{ 0x1.2387a6p0f, 0x1.ceac48p-25f },
	{ 0x1.29e9ep0f, -0x1.5c0424p-25f },
	{ 0x1.306fep0f, 0x1.4636e2p-25f },
	{ 0x1.371a74p0f, -0x1.18aac6p-25f },
	{ 0x1.3dea64p0f, 0x1.824684p-25f },
	{ 0x1.44e086p0f, 0x1.8624b4p-30f },
	{ 0x1.4bfdaep0f, -0x1.593abcp-25f },
	{ 0x1.5342b6p0f, -0x1.2c561p-25f },
	{ 0x1.5ab07ep0f, -0x1.5bd5ecp-27f },
	{ 0x1.6247ecp0f, -0x1.f8b55p-25f },
	{ 0x1.6a09e6p0f, 0x1.9fcef4p-26f },
	{ 0x1.71f75ep0f, 0x1.1d8beep-25f },
	{ 0x1.7a1148p0f, -0x1.829fdp-25f },
	{ 0x1.82589ap0f, -0x1.accc7cp-26f },
	{ 0x1.8ace54p0f, 0x1.15506ep-27f },
	{ 0x1.93737cp0f, -0x1.e64744p-25f },
	{ 0x1.9c4918p0f, 0x1.51f848p-27f },
	{ 0x1.a5503cp0f, -0x1.b83b54p-25f },
	{ 0x1.ae89fap0f, -0x1.a94b14p-26f },
	{ 0x1.b7f77p0f, -0x1.a09438p-25f },
	{ 0x1.c199bep0f, -0x1.3d56b2p-27f },
	{ 0x1.cb720ep0f, -0x1.8837ccp-27f },
	{ 0x1.d5818ep0f, -0x1.822dbcp-27f },
	{ 0x1.dfc974p0f, -0x1.908c94p-25f },
	{ 0x1.ea4afap0f, 0x1.52486cp-27f },
	{ 0x1.f50766p0f, -0x1.246ebp-26f }
};

/*
 * 2^z = 2^(k/32) * 2^d with k = round(32 z) and |d| <= 1/64: 2^(k/32) is
 * 2^floor(k/32) times a table entry, and 2^d = e^r with r = d ln 2, |r| below
 * 0.011, a short Taylor series. d = (z.hi - k/32) + z.lo is exact (the
 * subtraction by Sterbenz, the sum as float-float); every product that
 * carries the leading bits is exact (Dekker), so the result is good to about
 * 2^-40 and the caller rounds it once (PLAN D57).
 */
struct __libc_ff __libc_exp2f_core(struct __libc_ff z, int *exponent)
{
	/* Round to nearest integer: the only direction of the cores this layer serves */
	float kf = (z.hi * 32.0f + 0x1.8p23f) - 0x1.8p23f;
	int k = (int)kf;
	int j = k & 31;
	struct __libc_ff d = __libc_ff_fast_sum(z.hi - kf * 0x1p-5f, z.lo);
	struct __libc_ff e = __libc_ff_expm1_series(__libc_ff_multiply(d, __LIBC_LN2F));
	float th = table[j][0];
	float tl = table[j][1];
	struct __libc_ff product = __libc_ff_product(th, e.hi);
	struct __libc_ff sum = __libc_ff_fast_sum(th, product.hi);

	/* floor(k / 32), without shifting a negative value */
	*exponent = (k - j) / 32;

	return __libc_ff_fast_sum(sum.hi, sum.lo + (product.lo + (th * e.lo + (tl + tl * e.hi))));
}
