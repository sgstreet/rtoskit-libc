/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __muldc3.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "softdouble.h"

/*
 * (a + bi)(c + di), the compiler's helper for complex multiplication
 * (C23 6.5.6, Annex G.5.2; PLAN D100). The textbook formula, and when both
 * parts come out NaN, the recovery of G.5.2p1: an infinite operand times a
 * nonzero finite or infinite one is an infinity. The infinite parts become
 * signed ones and the finite parts of the other operand stay, its NaN
 * parts become signed zeros, and an overflowed partial product counts as
 * infinite. libgcc's version would bring its own double helpers with it.
 */
double complex __muldc3(double a, double b, double c, double d)
{
	double ac = a * c;
	double bd = b * d;
	double ad = a * d;
	double bc = b * c;
	double x = ac - bd;
	double y = ad + bc;
	bool recalculate = false;

	if (!isnan(x) || !isnan(y))
		return CMPLX(x, y);

	if (isinf(a) || isinf(b)) {
		a = __builtin_copysign(isinf(a) ? 1.0 : 0.0, a);
		b = __builtin_copysign(isinf(b) ? 1.0 : 0.0, b);
		if (isnan(c))
			c = __builtin_copysign(0.0, c);
		if (isnan(d))
			d = __builtin_copysign(0.0, d);
		recalculate = true;
	}
	if (isinf(c) || isinf(d)) {
		c = __builtin_copysign(isinf(c) ? 1.0 : 0.0, c);
		d = __builtin_copysign(isinf(d) ? 1.0 : 0.0, d);
		if (isnan(a))
			a = __builtin_copysign(0.0, a);
		if (isnan(b))
			b = __builtin_copysign(0.0, b);
		recalculate = true;
	}
	if (!recalculate && (isinf(ac) || isinf(bd) || isinf(ad) || isinf(bc))) {
		if (isnan(a))
			a = __builtin_copysign(0.0, a);
		if (isnan(b))
			b = __builtin_copysign(0.0, b);
		if (isnan(c))
			c = __builtin_copysign(0.0, c);
		if (isnan(d))
			d = __builtin_copysign(0.0, d);
		recalculate = true;
	}
	if (recalculate) {
		x = INFINITY * (a * c - b * d);
		y = INFINITY * (a * d + b * c);
	}

	return CMPLX(x, y);
}
