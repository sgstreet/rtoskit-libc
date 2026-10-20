/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * llrint.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <math.h>

#include "internal/math.h"

/*
 * C23 7.12.9.5, F.10.6: outside the range of long long the result is unspecified
 * (LLONG_MIN) and invalid is raised; otherwise inexact when the value changed (rint raises it).
 */
long long llrint(double x)
{
	double limit = 0x1p63;
	double rounded = rint(x);

	if (!(rounded >= -limit && rounded < limit)) {
		__libc_math_raise_invalid();
		return LLONG_MIN;
	}

	return __libc_double_to_i64(rounded);
}
