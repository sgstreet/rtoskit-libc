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

/* 2^(j/8) as float-float, j = 0 .. 7 */
static const float table[8][2] = {
	{ 0x1p0f, 0.0f },
	{ 0x1.172b84p0f, -0x1.c15742p-27f },
	{ 0x1.306fep0f, 0x1.4636e2p-25f },
	{ 0x1.4bfdaep0f, -0x1.593abcp-25f },
	{ 0x1.6a09e6p0f, 0x1.9fcef4p-26f },
	{ 0x1.8ace54p0f, 0x1.15506ep-27f },
	{ 0x1.ae89fap0f, -0x1.a94b14p-26f },
	{ 0x1.d5818ep0f, -0x1.822dbcp-27f }
};

/*
 * 2^z = 2^(k/8) * 2^d with k = round(8 z) and |d| <= 1/16: 2^(k/8) is
 * 2^floor(k/8) times a table entry, and 2^d = e^r with r = d ln 2, |r| below
 * 0.044, a short Taylor series with an exact square (PLAN D68: the table was
 * 32 entries before the exact square made the larger range as accurate). d = (z.hi - k/32) + z.lo is exact (the
 * subtraction by Sterbenz, the sum as float-float); every product that
 * carries the leading bits is exact (Dekker), so the result is good to about
 * 2^-40 and the caller rounds it once (PLAN D57).
 */
struct __libc_ff __libc_exp2f_core(struct __libc_ff z, int *exponent)
{
	/* Round to nearest integer: the only direction of the cores this layer serves */
	float kf = (z.hi * 8.0f + 0x1.8p23f) - 0x1.8p23f;
	int k = (int)kf;
	int j = k & 7;
	struct __libc_ff d = __libc_ff_fast_sum(z.hi - kf * 0x1p-3f, z.lo);
	struct __libc_ff e = __libc_ff_expm1_series(__libc_ff_multiply(d, __LIBC_LN2F));
	float th = table[j][0];
	float tl = table[j][1];
	struct __libc_ff product = __libc_ff_product(th, e.hi);
	struct __libc_ff sum = __libc_ff_fast_sum(th, product.hi);

	/* floor(k / 8), without shifting a negative value */
	*exponent = (k - j) / 8;

	return __libc_ff_fast_sum(sum.hi, sum.lo + (product.lo + (th * e.lo + (tl + tl * e.hi))));
}
