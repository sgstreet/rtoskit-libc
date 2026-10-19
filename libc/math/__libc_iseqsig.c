/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_iseqsig.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.17.7: x == y, raising invalid for any NaN (a signaling comparison) */
int __libc_iseqsig(double x, double y)
{
	if (isnan(x) || isnan(y)) {
		__libc_math_raise_invalid();
		return 0;
	}

	return x == y;
}
