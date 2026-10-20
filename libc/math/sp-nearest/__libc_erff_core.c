/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_erff_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * erf(x) for 0 <= x < 4 as float-float: (2/sqrt(pi)) e^-x^2 sum 2^n
 * x^(2n+1)/(2n+1)!!. Every term is positive, so the sum cancels nothing at
 * any x; it ends once the terms have peaked (n >= x^2) and fallen below
 * 2^-46 of the sum (55 terms at 4). e^-x^2 comes from the exp2 core with
 * the square exact, to about 2^-42 in all (PLAN D63).
 */
struct __libc_ff __libc_erff_core(float x)
{
	struct __libc_ff square = __libc_ff_product(x, x);
	struct __libc_ff twice = { .hi = 2.0f * square.hi, .lo = 2.0f * square.lo };
	struct __libc_ff term = { .hi = x, .lo = 0.0f };
	struct __libc_ff sum = term;
	struct __libc_ff decay;
	int exponent;

	for (int n = 1; (float)n < square.hi || term.hi > sum.hi * 0x1p-46f; ++n) {
		term = __libc_ff_divide(__libc_ff_multiply(term, twice), (struct __libc_ff){ .hi = (float)(2 * n + 1), .lo = 0.0f });
		sum = __libc_ff_add(sum, term);
	}

	decay = __libc_exp2f_core(__libc_ff_multiply((struct __libc_ff){ .hi = -square.hi, .lo = -square.lo }, __LIBC_INV_LN2F), &exponent);
	decay = (struct __libc_ff){ .hi = scalbnf(decay.hi, exponent), .lo = scalbnf(decay.lo, exponent) };

	return __libc_ff_multiply(__libc_ff_multiply(sum, decay), __LIBC_TWO_SQRTPIF);
}
