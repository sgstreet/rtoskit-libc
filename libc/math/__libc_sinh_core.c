/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_sinh_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * sinh(x) for |x| < 1, the Taylor series: x + x^3/6 with the cube and the
 * sixth as double-doubles, then x^5 ... x^21 (the next term is below 2^-74).
 */
struct __libc_dd __libc_sinh_core(double x)
{
	const struct __libc_dd sixth = { .hi = 0x1.5555555555555p-3, .lo = 0x1.5555555555555p-57 };
	double z = x * x;
	struct __libc_dd square = __libc_dd_product(x, x);
	struct __libc_dd term = __libc_dd_multiply(__libc_dd_multiply(square, (struct __libc_dd){ .hi = x, .lo = 0.0 }), sixth);
	double tail = x * z * z * (0x1.1111111111111p-7 + z * (0x1.a01a01a01a01ap-13 + z * (0x1.71de3a556c734p-19 + z * (0x1.ae64567f544e4p-26 + z * (0x1.6124613a86d09p-33 + z * (0x1.ae7f3e733b81fp-41 + z * (0x1.952c77030ad4ap-49 + z * (0x1.2f49b46814157p-57 + z * 0x1.71b8ef6dcf572p-66))))))));
	struct __libc_dd sum = __libc_dd_fast_sum(x, term.hi);

	return __libc_dd_fast_sum(sum.hi, sum.lo + term.lo + tail);
}
