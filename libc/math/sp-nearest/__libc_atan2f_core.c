/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_atan2f_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* atan(k/16), k = 0 .. 16, as float-float */
static const float table[17][2] = {
	{ 0.0f, 0.0f },
	{ 0x1.ff55bcp-5f, -0x1.1a6042p-30f },
	{ 0x1.fd5baap-4f, -0x1.54f424p-30f },
	{ 0x1.7b97b4p-3f, 0x1.79cb6p-28f },
	{ 0x1.f5b76p-3f, -0x1.b4dfc8p-29f },
	{ 0x1.362774p-2f, -0x1.1f0286p-27f },
	{ 0x1.6f6194p-2f, 0x1.e4defp-30f },
	{ 0x1.a64eecp-2f, 0x1.e611fep-29f },
	{ 0x1.dac67p-2f, 0x1.586ed4p-28f },
	{ 0x1.0657eap-1f, -0x1.6499e6p-26f },
	{ 0x1.1e00bap-1f, 0x1.7bdfd6p-26f },
	{ 0x1.345f02p-1f, -0x1.98e422p-28f },
	{ 0x1.4978fap-1f, 0x1.934f7p-28f },
	{ 0x1.5d5898p-1f, 0x1.c5a6c6p-27f },
	{ 0x1.700a7cp-1f, 0x1.5e118cp-27f },
	{ 0x1.819d0cp-1f, -0x1.1d4eb6p-26f },
	{ 0x1.921fb6p-1f, -0x1.777a5cp-26f },
};

/*
 * atan(t) for t in [0, 1] (Tang): c = k/16 nearest t, atan(t) = atan(c) +
 * atan(u) with u = (t - c)/(1 + t c), |u| <= 1/32 (t - c exact by Sterbenz,
 * the product exact), and atan(u) = u - u^3/3 + u^5/5 - u^7/7 + u^9/9 with
 * the cube and the 1/3 as float-float, to about 2^-42.
 */
static struct __libc_ff atan_unit(struct __libc_ff t)
{
	int k = (int)(t.hi * 16.0f + 0.5f);
	float c = (float)k * 0x1p-4f;
	struct __libc_ff numerator = __libc_ff_fast_sum(t.hi - c, t.lo);
	struct __libc_ff product = __libc_ff_product(t.hi, c);
	struct __libc_ff denominator = __libc_ff_fast_sum(1.0f, product.hi);
	struct __libc_ff u;
	struct __libc_ff square;
	struct __libc_ff series;
	float v;

	denominator = __libc_ff_fast_sum(denominator.hi, denominator.lo + product.lo + t.lo * c);
	u = __libc_ff_divide(numerator, denominator);
	square = __libc_ff_product(u.hi, u.hi);
	square.lo += 2.0f * u.hi * u.lo;
	v = square.hi;
	series = __libc_ff_multiply(__libc_ff_multiply(u, square), __libc_ff_fast_sum(-0x1.555556p-2f, 0x1.555556p-27f + v * (0x1.99999ap-3f - v * (0x1.24924ap-3f - v * 0x1.c71c72p-4f))));

	return __libc_ff_add((struct __libc_ff){ .hi = table[k][0], .lo = table[k][1] }, __libc_ff_add(u, series));
}

/*
 * atan2(y, x) for finite y, x not both zero: the smaller coordinate over
 * the larger, then pi/2 - a when they were swapped, pi - a for a negative
 * x, and the sign of y (PLAN D61).
 */
struct __libc_ff __libc_atan2f_core(struct __libc_ff y, struct __libc_ff x)
{
	struct __libc_ff ay = signbit(y.hi) ? (struct __libc_ff){ .hi = -y.hi, .lo = -y.lo } : y;
	struct __libc_ff ax = signbit(x.hi) ? (struct __libc_ff){ .hi = -x.hi, .lo = -x.lo } : x;
	bool swap = ay.hi > ax.hi;
	struct __libc_ff angle = swap ? atan_unit(__libc_ff_divide(ax, ay)) : atan_unit(__libc_ff_divide(ay, ax));

	if (swap)
		angle = __libc_ff_add(__LIBC_PI_2F, (struct __libc_ff){ .hi = -angle.hi, .lo = -angle.lo });
	if (signbit(x.hi))
		angle = __libc_ff_add(__LIBC_PIF, (struct __libc_ff){ .hi = -angle.hi, .lo = -angle.lo });

	return signbit(y.hi) ? (struct __libc_ff){ .hi = -angle.hi, .lo = -angle.lo } : angle;
}
