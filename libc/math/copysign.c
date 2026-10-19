/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * copysign.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.11.1: the magnitude of x and the sign of y, also for NaNs */
double copysign(double x, double y)
{
	return __libc_double_from((__libc_double_bits(x) & ~__LIBC_DOUBLE_SIGN) | (__libc_double_bits(y) & __LIBC_DOUBLE_SIGN));
}
