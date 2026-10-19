/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_expm1_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * expm1 of a double-double argument: the Taylor series for |z| below 2^-10
 * (remainder below 2^-80); otherwise exp(z) - 1 with the subtraction done
 * exactly on the high part (Sterbenz when the scale is 1) and the low part
 * added last, so one rounding gives the result.
 */
double __libc_expm1_core(double zh, double zl)
{
	struct __libc_dd value;
	struct __libc_dd shifted;
	int exponent;
	int mode;

	if (fabs(zh) < 0x1p-10)
		return zh + (zl + zh * zl + zh * zh * (0.5 + zh * (0x1.5555555555555p-3 + zh * (0x1.5555555555555p-5 + zh * (0x1.1111111111111p-7 + zh * (0x1.6c16c16c16c17p-10 + zh * 0x1.a01a01a01a01ap-13))))));

	value = __libc_exp_core(zh, zl, &exponent);
	if (exponent == 0)
		return (value.hi - 1.0) + value.lo;
	if (exponent > 60)
		return __libc_exp_scale(value, exponent);

	mode = __libc_math_nearest();
	shifted = __libc_dd_sum(scalbn(value.hi, exponent), -1.0);
	__libc_math_restore(mode);
	return shifted.hi + (shifted.lo + scalbn(value.lo, exponent));
}
