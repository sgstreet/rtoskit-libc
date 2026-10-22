/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __divdc3.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "softdouble.h"

/*
 * (a + bi)/(c + di), the compiler's helper for complex division (C23
 * 6.5.6, Annex G.5.2; PLAN D100). The divisor is scaled by a power of two
 * near its larger part, so that c^2 + d^2 neither overflows nor underflows,
 * and the quotient scaled back. Scaling by a power of two is exact while
 * nothing overflows or underflows. When every part is zero or within
 * 2^-128 to 2^128, every product, numerator, denominator and quotient,
 * scaled or not, stays in the normal range, so the unscaled formula gives
 * the same bits, without the logb and scalbn calls. When both parts come out NaN, G.5.2p1's
 * cases are recovered: a nonzero or infinite dividend over zero is an
 * infinity, an infinite dividend over a finite divisor is an infinity, and
 * a finite dividend over an infinite divisor is a zero. libgcc's version
 * would bring its own double helpers with it.
 */
static bool moderate(double x)
{
	uint32_t field = (uint32_t)(__libc_softdouble_bits(x) >> 52) & 0x7ff;

	return field - (1023 - 128) <= 256 || (__libc_softdouble_bits(x) << 1) == 0;
}

double complex __divdc3(double a, double b, double c, double d)
{
	double scale = 0.0;
	int exponent = 0;
	double denominator;
	double x;
	double y;

	if (!moderate(a) || !moderate(b) || !moderate(c) || !moderate(d))
		scale = logb(__builtin_fmax(__builtin_fabs(c), __builtin_fabs(d)));
	if (scale != 0.0 && isfinite(scale)) {
		exponent = (int)scale;
		c = scalbn(c, -exponent);
		d = scalbn(d, -exponent);
	}
	denominator = c * c + d * d;
	x = scalbn((a * c + b * d) / denominator, -exponent);
	y = scalbn((b * c - a * d) / denominator, -exponent);
	if (!isnan(x) || !isnan(y))
		return CMPLX(x, y);

	if (denominator == 0.0 && (!isnan(a) || !isnan(b))) {
		x = __builtin_copysign(INFINITY, c) * a;
		y = __builtin_copysign(INFINITY, c) * b;
	} else if ((isinf(a) || isinf(b)) && isfinite(c) && isfinite(d)) {
		a = __builtin_copysign(isinf(a) ? 1.0 : 0.0, a);
		b = __builtin_copysign(isinf(b) ? 1.0 : 0.0, b);
		x = INFINITY * (a * c + b * d);
		y = INFINITY * (b * c - a * d);
	} else if (isinf(scale) && scale > 0.0 && isfinite(a) && isfinite(b)) {
		c = __builtin_copysign(isinf(c) ? 1.0 : 0.0, c);
		d = __builtin_copysign(isinf(d) ? 1.0 : 0.0, d);
		x = 0.0 * (a * c + b * d);
		y = 0.0 * (b * c - a * d);
	}

	return CMPLX(x, y);
}
