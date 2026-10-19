/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_cos_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * cos(r) for |r| <= 0.8: 1 - r^2/2 with the square exact (and 2 r.hi r.lo
 * for the low part), then the Taylor terms r^4 ... r^18 (the next one is
 * below 2^-67).
 */
struct __libc_dd __libc_cos_core(struct __libc_dd r)
{
	double x = r.hi;
	double z = x * x;
	struct __libc_dd square = __libc_dd_product(x, x);
	double tail = z * z * (0x1.5555555555555p-5 + z * (-0x1.6c16c16c16c17p-10 + z * (0x1.a01a01a01a01ap-16 + z * (-0x1.27e4fb7789f5cp-22 + z * (0x1.1eed8eff8d898p-29 + z * (-0x1.93974a8c07c9dp-37 + z * (0x1.ae7f3e733b81fp-45 + z * -0x1.6827863b97d97p-53)))))));
	struct __libc_dd sum = __libc_dd_fast_sum(1.0, -0.5 * square.hi);

	return __libc_dd_fast_sum(sum.hi, sum.lo - 0.5 * square.lo - x * r.lo + tail);
}
