/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __divsc3.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "softdouble.h"

/*
 * (a + bi)/(c + di) in float, the compiler's helper for float complex
 * division (C23 6.5.6, Annex G.5.2; PLAN D100), computed in double: the
 * products of floats are exact there, and c^2 + d^2 lies between 2^-298
 * and 2^256, so no scaling is needed; each part is then rounded to float.
 * When both parts come out NaN, G.5.2p1's cases are recovered as in
 * __divdc3. libgcc's version would bring its own double helpers with it.
 */
float complex __divsc3(float a, float b, float c, float d)
{
	double denominator = (double)c * c + (double)d * d;
	double x = ((double)a * c + (double)b * d) / denominator;
	double y = ((double)b * c - (double)a * d) / denominator;

	if (!isnan(x) || !isnan(y))
		return CMPLXF((float)x, (float)y);

	if (denominator == 0.0 && (!isnan(a) || !isnan(b))) {
		x = __builtin_copysign(INFINITY, c) * a;
		y = __builtin_copysign(INFINITY, c) * b;
	} else if ((isinf(a) || isinf(b)) && isfinite(c) && isfinite(d)) {
		double p = __builtin_copysign(isinf(a) ? 1.0 : 0.0, a);
		double q = __builtin_copysign(isinf(b) ? 1.0 : 0.0, b);

		x = INFINITY * (p * c + q * d);
		y = INFINITY * (q * c - p * d);
	} else if ((isinf(c) || isinf(d)) && isfinite(a) && isfinite(b)) {
		double p = __builtin_copysign(isinf(c) ? 1.0 : 0.0, c);
		double q = __builtin_copysign(isinf(d) ? 1.0 : 0.0, d);

		x = 0.0 * (a * p + b * q);
		y = 0.0 * (b * p - a * q);
	}

	return CMPLXF((float)x, (float)y);
}
