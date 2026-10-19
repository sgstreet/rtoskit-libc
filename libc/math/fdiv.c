/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fdiv.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.14.3: x / y rounded to float once: 64 quotient bits by long
 * division of the significands, the remainder as the sticky bit.
 */
float fdiv(double x, double y)
{
	struct __libc_exact quotient;
	int ex;
	int ey;
	uint64_t mx;
	uint64_t my;
	uint64_t bits = 0;

	if (!isfinite(x) || !isfinite(y) || x == 0 || y == 0)
		return (float)(x / y);

	mx = __libc_double_split(x, &ex);
	my = __libc_double_split(y, &ey);
	for (int i = 0; i < 64; ++i) {
		bits <<= 1;
		if (mx >= my) {
			mx -= my;
			bits |= 1;
		}
		mx <<= 1;
	}

	quotient = (struct __libc_exact){ .negative = signbit(x) != signbit(y), .sticky = mx != 0, .exponent = ex - ey - 63, .high = 0, .low = bits };
	return __libc_exact_to_float(&quotient);
}
