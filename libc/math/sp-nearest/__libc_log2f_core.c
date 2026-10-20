/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_log2f_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* For c = 1 + j/32, j = -9 .. 13: 1/c rounded to float, and -log2 of that float as float-float */
static const float table[23][3] = {
	{ 0x1.642c86p0f, -0x1.e7df62p-2f, 0x1.347704p-28f }, /* -9 */
	{ 0x1.555556p0f, -0x1.a8ff9ap-2f, 0x1.519b8cp-32f }, /* -8 */
	{ 0x1.47ae14p0f, -0x1.6cb0f4p-2f, -0x1.717772p-28f }, /* -7 */
	{ 0x1.3b13b2p0f, -0x1.32bff2p-2f, 0x1.6cef96p-29f }, /* -6 */
	{ 0x1.2f684cp0f, -0x1.f5fd8cp-3f, -0x1.b85984p-35f }, /* -5 */
	{ 0x1.24924ap0f, -0x1.8a898ap-3f, 0x1.5812d4p-28f }, /* -4 */
	{ 0x1.1a7b96p0f, -0x1.22dadcp-3f, 0x1.1bede4p-28f }, /* -3 */
	{ 0x1.111112p0f, -0x1.7d605ep-4f, 0x1.81976ep-30f }, /* -2 */
	{ 0x1.08421p0f, -0x1.773936p-5f, 0x1.dec776p-31f }, /* -1 */
	{ 0x1p0f, 0.0f, 0.0f }, /* 0 */
	{ 0x1.f07c2p-1f, 0x1.6bad2p-5f, 0x1.0ea1e4p-31f }, /* 1 */
	{ 0x1.e1e1e2p-1f, 0x1.663f6ep-4f, 0x1.d9e5dap-31f }, /* 2 */
	{ 0x1.d41d42p-1f, 0x1.08c588p-3f, -0x1.1d5eeap-29f }, /* 3 */
	{ 0x1.c71c72p-1f, 0x1.5c01a2p-3f, 0x1.734792p-30f }, /* 4 */
	{ 0x1.bacf92p-1f, 0x1.acf5dep-3f, 0x1.57e24ep-30f }, /* 5 */
	{ 0x1.af286cp-1f, 0x1.fbc16ap-3f, 0x1.ed20a6p-31f }, /* 6 */
	{ 0x1.a41a42p-1f, 0x1.24407ap-2f, -0x1.2492e8p-27f }, /* 7 */
	{ 0x1.99999ap-1f, 0x1.49a784p-2f, -0x1.69057ap-27f }, /* 8 */
	{ 0x1.8f9c18p-1f, 0x1.6e222p-2f, 0x1.d48fdcp-28f }, /* 9 */
	{ 0x1.861862p-1f, 0x1.91bba6p-2f, 0x1.888fbap-27f }, /* 10 */
	{ 0x1.7d05f4p-1f, 0x1.b47ecp-2f, -0x1.8115c2p-29f }, /* 11 */
	{ 0x1.745d18p-1f, 0x1.d6753cp-2f, -0x1.bef496p-27f }, /* 12 */
	{ 0x1.6c16c2p-1f, 0x1.f7a854p-2f, 0x1.a4396ap-29f }, /* 13 */
};

/*
 * log2(x + tail) = e + log2(1/r) + log2(1 + f) with x = 2^e m, m in
 * [sqrt(1/2), sqrt(2)), r the table's 1/c for the c nearest m and
 * f = m r - 1: the product is exact (Dekker) and the subtraction exact
 * (Sterbenz), so f is exact as float-float and |f| < 0.022. Near 1 (e = 0,
 * c = 1) the result keeps f's relative accuracy; elsewhere the parts add to
 * about 2^-40 before the caller's one rounding (PLAN D58).
 */
struct __libc_ff __libc_log2f_core(float x, float tail)
{
	uint32_t bits = __libc_float_bits(x);
	int exponent = __libc_float_exponent(bits);
	float m;
	float jf;
	int j;
	struct __libc_ff product;
	struct __libc_ff f;
	struct __libc_ff sum;

	/* A subnormal x is scaled into the normal range, exactly */
	if (exponent == 0) {
		bits = __libc_float_bits(x * 0x1p23f);
		tail *= 0x1p23f;
		exponent = __libc_float_exponent(bits) - 23;
	}
	exponent -= __LIBC_FLOAT_BIAS;
	m = __libc_float_from((bits & __LIBC_FLOAT_FRACTION) | __libc_float_bits(1.0f));
	if (m > 0x1.6a09e6p0f) {
		m *= 0.5f;
		++exponent;
	}

	/* Round to nearest integer: the only direction of the cores this layer serves */
	jf = ((m - 1.0f) * 32.0f + 0x1.8p23f) - 0x1.8p23f;
	j = (int)jf + 9;
	product = __libc_ff_product(m, table[j][0]);
	f = __libc_ff_sum(product.hi - 1.0f, product.lo + scalbnf(tail, -exponent) * table[j][0]);
	sum = __libc_ff_add((struct __libc_ff){ .hi = table[j][1], .lo = table[j][2] }, __libc_ff_multiply(__libc_ff_log1p_series(f), __LIBC_INV_LN2F));

	return __libc_ff_add((struct __libc_ff){ .hi = (float)exponent, .lo = 0.0f }, sum);
}
