/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_sin_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * sin(r) for |r| <= 0.8: r - r^3/6 with the cube and the sixth as
 * double-doubles, then the Taylor terms r^5 ... r^17 (the next one is below
 * 2^-62 relative); the low part of r enters through cos(r) ~ 1 - r^2/2.
 */
struct __libc_dd __libc_sin_core(struct __libc_dd r)
{
	const struct __libc_dd sixth = { .hi = 0x1.5555555555555p-3, .lo = 0x1.5555555555555p-57 };
	double x = r.hi;
	double z = x * x;
	struct __libc_dd square = __libc_dd_product(x, x);
	struct __libc_dd cube = __libc_dd_multiply(square, (struct __libc_dd){ .hi = x, .lo = 0.0 });
	struct __libc_dd term = __libc_dd_multiply(cube, sixth);
	double tail = x * z * z * (0x1.1111111111111p-7 + z * (-0x1.a01a01a01a01ap-13 + z * (0x1.71de3a556c734p-19 + z * (-0x1.ae64567f544e4p-26 + z * (0x1.6124613a86d09p-33 + z * (-0x1.ae7f3e733b81fp-41 + z * 0x1.952c77030ad4ap-49))))));
	struct __libc_dd sum = __libc_dd_fast_sum(x, -term.hi);

	return __libc_dd_fast_sum(sum.hi, sum.lo - term.lo + tail + r.lo * (1.0 - 0.5 * z));
}
