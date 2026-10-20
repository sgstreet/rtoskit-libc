/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_sincosf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * sin y and cos y as float-float for a finite float-float y: the exact
 * reduction of y.hi from 2^-6 up, y.lo joining the reduced angle, then the
 * sin core at k and k + 16 (PLAN D60, D65). Below 2^-20 sin y is y and
 * cos y is 1 - y^2/2 (the next terms are below 2^-42), which keeps a
 * subnormal y exact where the core would round on the subnormal grid.
 */
void __libc_sincosf_core(struct __libc_ff y, struct __libc_ff *sine, struct __libc_ff *cosine)
{
	struct __libc_ff s = y;
	int k = 0;

	if (fabsf(y.hi) < 0x1p-20f) {
		*sine = s;
		*cosine = (struct __libc_ff){ .hi = 1.0f, .lo = -0.5f * y.hi * y.hi };
		return;
	}
	if (fabsf(y.hi) >= 0x1p-6f) {
		k = __libc_rem_pio2f(y.hi, &s);
		s = __libc_ff_add(s, (struct __libc_ff){ .hi = y.lo, .lo = 0.0f });
	}

	*sine = __libc_sinf_core(k, s);
	*cosine = __libc_sinf_core(k + 16, s);
}
