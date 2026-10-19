/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_erfc_tail.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * erfc(x) = exp(-x^2) / sqrt(pi) / K(x) for 4 <= x < 27.3, K the continued
 * fraction x + (1/2)/(x + 1/(x + (3/2)/(x + ...))) evaluated backwards from 30
 * levels (below 2^-68 relative from x = 4) as double-doubles; x^2 is exact.
 */
struct __libc_dd __libc_erfc_tail(double x, int *exponent)
{
	const struct __libc_dd inv_sqrtpi = { .hi = 0x1.20dd750429b6dp-1, .lo = 0x1.1ae3a914fed80p-57 };
	struct __libc_dd square = __libc_dd_product(x, x);
	struct __libc_dd fraction = { .hi = x, .lo = 0.0 };
	struct __libc_dd value;

	for (int k = 30; k >= 1; --k)
		fraction = __libc_dd_add((struct __libc_dd){ .hi = x, .lo = 0.0 }, __libc_dd_divide((struct __libc_dd){ .hi = 0.5 * k, .lo = 0.0 }, fraction));
	value = __libc_exp_core(-square.hi, -square.lo, exponent);

	return __libc_dd_multiply(value, __libc_dd_divide(inv_sqrtpi, fraction));
}
