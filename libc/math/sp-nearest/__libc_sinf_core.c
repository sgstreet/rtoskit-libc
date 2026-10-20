/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_sinf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* sin and cos of j pi/32 as float-float, j = 0 .. 8 */
static const float table[9][4] = {
	{ 0.0f, 0.0f, 0x1p0f, 0.0f },
	{ 0x1.917a6cp-4f, -0x1.eb25eap-31f, 0x1.fd88dap-1f, 0x1.e89292p-28f },
	{ 0x1.8f8b84p-3f, -0x1.cb2cfap-30f, 0x1.f6297cp-1f, 0x1.feeb96p-26f },
	{ 0x1.294062p-2f, 0x1.dab3ep-27f, 0x1.e9f416p-1f, -0x1.273a44p-26f },
	{ 0x1.87de2ap-2f, 0x1.abaa58p-28f, 0x1.d906bcp-1f, 0x1.e651a8p-26f },
	{ 0x1.e2b5d4p-2f, -0x1.fe4272p-28f, 0x1.c38b3p-1f, -0x1.cfe84ap-26f },
	{ 0x1.1c73b4p-1f, -0x1.9465cep-27f, 0x1.a9b662p-1f, 0x1.21d434p-26f },
	{ 0x1.44cf32p-1f, 0x1.424776p-27f, 0x1.8bc806p-1f, 0x1.62a2e8p-26f },
	{ 0x1.6a09e6p-1f, 0x1.9fcef4p-27f, 0x1.6a09e6p-1f, 0x1.9fcef4p-27f },
};

/*
 * sin(k pi/32 + s) for any k (taken modulo 64) and |s| <= pi/64 (Tang):
 * k + 8 = 16 q + (j + 8) modulo 64 gives the quadrant q, j in [-8, 7] and the table angle
 * a = |j| pi/32; sin(s) = s - s^3/6 + s^5/120 with the cube and the 1/6 as
 * float-float, cos(s) - 1 = -s^2/2 + s^4/24 - s^6/720 from an exact square,
 * and the angle sums sin a cos s + cos a sin s (or the cosine's) as
 * float-float products, to about 2^-42 before the caller's one rounding
 * (PLAN D60).
 */
struct __libc_ff __libc_sinf_core(int k, struct __libc_ff s)
{
	int quadrant = ((k + 8) & 63) / 16;
	int j = ((k + 8) & 15) - 8;
	int a = j < 0 ? -j : j;
	float sign = j < 0 ? -1.0f : 1.0f;
	struct __libc_ff square = __libc_ff_product(s.hi, s.hi);
	struct __libc_ff z = __libc_ff_fast_sum(square.hi, square.lo + 2.0f * s.hi * s.lo);
	float tail = z.hi * (0x1.111112p-7f - z.hi * 0x1.a01a02p-13f);
	struct __libc_ff sine = __libc_ff_add(s, __libc_ff_multiply(__libc_ff_multiply(s, z), __libc_ff_fast_sum(-0x1.555556p-3f, 0x1.555556p-28f + tail)));
	struct __libc_ff cosine_m1 = __libc_ff_fast_sum(-0.5f * z.hi, -0.5f * z.lo + z.hi * z.hi * (0x1.555556p-5f - z.hi * 0x1.6c16c2p-10f));
	struct __libc_ff first;
	struct __libc_ff second;
	struct __libc_ff value;

	if ((quadrant & 1) == 0) {
		/* sin(a sign + s) = sign (sin a (1 + cm) + cos a sin(s) sign) */
		first = (struct __libc_ff){ .hi = sign * table[a][0], .lo = sign * table[a][1] };
		second = (struct __libc_ff){ .hi = table[a][2], .lo = table[a][3] };
	} else {
		/* cos(a sign + s) = cos a (1 + cm) - sin a sin(s) sign */
		first = (struct __libc_ff){ .hi = table[a][2], .lo = table[a][3] };
		second = (struct __libc_ff){ .hi = -sign * table[a][0], .lo = -sign * table[a][1] };
	}
	value = __libc_ff_add(__libc_ff_add(first, __libc_ff_multiply(first, cosine_m1)), __libc_ff_multiply(second, sine));

	return quadrant >= 2 ? (struct __libc_ff){ .hi = -value.hi, .lo = -value.lo } : value;
}
