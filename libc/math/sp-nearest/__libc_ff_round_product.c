/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_ff_round_product.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * 2^exponent a b rounded once. A factor below 2^-60 is scaled by 2^64 first:
 * the Dekker product of a subnormal loses its low bits, and a part made of
 * a tiny factor (sinh of a tiny x, sin of a tiny y) must keep its relative
 * accuracy. A zero factor gives the signed zero of the plain product: the
 * float-float sums would turn -0 into +0 (PLAN D65).
 */
float __libc_ff_round_product(struct __libc_ff a, struct __libc_ff b, int exponent)
{
	if (a.hi == 0 || b.hi == 0)
		return a.hi * b.hi;
	if (fabsf(a.hi) < 0x1p-60f) {
		a = (struct __libc_ff){ .hi = scalbnf(a.hi, 64), .lo = scalbnf(a.lo, 64) };
		exponent -= 64;
	}
	if (fabsf(b.hi) < 0x1p-60f) {
		b = (struct __libc_ff){ .hi = scalbnf(b.hi, 64), .lo = scalbnf(b.lo, 64) };
		exponent -= 64;
	}

	return __libc_exp2f_scale(__libc_ff_multiply(a, b), exponent);
}
