/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ffma.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/*
 * C23 7.12.14.4: x * y + z rounded once. Infinities, NaNs and zeros take
 * the ordinary operations, which are exact or give the special results; an
 * exact zero sum takes the sign the current direction gives z - z.
 */
float ffma(double x, double y, double z)
{
	struct __libc_exact sum;
	int ex;
	int ey;
	int ez;
	uint64_t mx;
	uint64_t my;
	uint64_t mz;

	if (!isfinite(x) || !isfinite(y))
		return (float)(x * y + z);
	if (!isfinite(z))
		return (float)(isnan(z) ? z + z : z);
	if (x == 0 || y == 0)
		return (float)(x * y + z);
	if (z == 0)
		return (float)(fmul(x, y));

	mx = __libc_double_split(x, &ex);
	my = __libc_double_split(y, &ey);
	mz = __libc_double_split(z, &ez);
	if (!__libc_exact_fma(&sum, signbit(x) != signbit(y), mx, my, ex + ey, signbit(z), mz, ez))
		return (float)(z - z);

	return __libc_exact_to_float(&sum);
}
