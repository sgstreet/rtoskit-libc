/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_pow_integer.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * exp(n * log) for a 64-bit integer n: n is split into a high part with its
 * low 11 bits clear (at most 52 significant bits, exact as a double) and the
 * rest; both products with log.hi are exact double-doubles, so the sum keeps
 * double-double accuracy for every n.
 */
double __libc_pow_integer(struct __libc_dd log, long long n, bool negative)
{
	double sign = negative ? -1.0 : 1.0;
	double high = __libc_i64_to_double(n & ~2047LL);
	double low = __libc_i64_to_double(n - (n & ~2047LL));
	double estimate = (high + low) * log.hi;
	struct __libc_dd upper;
	struct __libc_dd lower;
	struct __libc_dd z;
	struct __libc_dd value;
	int exponent;

	if (estimate > 710)
		return __libc_math_overflow(sign);
	if (estimate < -746)
		return __libc_math_underflow(sign);

	upper = __libc_dd_product(high, log.hi);
	lower = __libc_dd_product(low, log.hi);
	z = __libc_dd_sum(upper.hi, lower.hi);
	z = __libc_dd_fast_sum(z.hi, z.lo + upper.lo + lower.lo + high * log.lo + low * log.lo);
	value = __libc_exp_core(z.hi, z.lo, &exponent);

	return __libc_exp_scale(negative ? __libc_dd_negate(value) : value, exponent);
}
