/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cexpf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.7.1, G.6.3.1: e^x (cos y + i sin y), each part one rounding of
 * the exp2 core times the float-float cosine or sine (PLAN D65), so a finite
 * part past expf's overflow threshold survives; the special cases first.
 */
float complex cexpf(float complex z)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	struct __libc_ff power;
	struct __libc_ff sine;
	struct __libc_ff cosine;

	if (y == 0)
		return CMPLXF(expf(x), y);
	if (isinf(x)) {
		if (x < 0)
			return isfinite(y) ? CMPLXF(0.0f * cosf(y), 0.0f * sinf(y)) : CMPLXF(0.0f, 0.0f);
		return isfinite(y) ? CMPLXF(x * cosf(y), x * sinf(y)) : CMPLXF(x, y - y);
	}
	if (isnan(x))
		return CMPLXF(x, x);
	if (!isfinite(y))
		return CMPLXF(y - y, y - y);

	/* Beyond +-200 every part overflows or vanishes; the clamp keeps the float-float product finite */
	power = __libc_ff_scale(fmaxf(fminf(x, 200.0f), -200.0f), __LIBC_INV_LN2F);
	__libc_sincosf_core((struct __libc_ff){ .hi = y, .lo = 0.0f }, &sine, &cosine);

	return CMPLXF(__libc_exp2f_times(power, cosine), __libc_exp2f_times(power, sine));
}
