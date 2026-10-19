/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_cosh_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* cosh(x) for |x| < 1, the Taylor series: 1 + x^2/2 with the square exact, then x^4 ... x^20 */
struct __libc_dd __libc_cosh_core(double x)
{
	double z = x * x;
	struct __libc_dd square = __libc_dd_product(x, x);
	double tail = z * z * (0x1.5555555555555p-5 + z * (0x1.6c16c16c16c17p-10 + z * (0x1.a01a01a01a01ap-16 + z * (0x1.27e4fb7789f5cp-22 + z * (0x1.1eed8eff8d898p-29 + z * (0x1.93974a8c07c9dp-37 + z * (0x1.ae7f3e733b81fp-45 + z * (0x1.6827863b97d97p-53 + z * 0x1.e542ba4020225p-62))))))));
	struct __libc_dd sum = __libc_dd_fast_sum(1.0, 0.5 * square.hi);

	return __libc_dd_fast_sum(sum.hi, sum.lo + 0.5 * square.lo + tail);
}
