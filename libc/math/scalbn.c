/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * scalbn.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.6.19: x * 2^n with one rounding. Large scales go in steps that
 * stay exact: up by 2^1023; down by 2^-969 (2^-1022 * 2^53), which keeps the
 * intermediate normal unless the result is far below the subnormal range,
 * where only its sign and stickiness still matter.
 */
double scalbn(double x, int n)
{
	if (n > 1023) {
		x *= 0x1p1023;
		n -= 1023;
		if (n > 1023) {
			x *= 0x1p1023;
			n -= 1023;
			if (n > 1023)
				n = 1023;
		}
	} else if (n < -1022) {
		x *= 0x1p-969;
		n += 969;
		if (n < -1022) {
			x *= 0x1p-969;
			n += 969;
			if (n < -1022)
				n = -1022;
		}
	}

	return x * __libc_double_from((uint64_t)(n + __LIBC_DOUBLE_BIAS) << 52);
}
