/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * csinhf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.6.5, G.6.2.5: sinh x cos y + i cosh x sin y, the special cases
 * first; each part one rounding of float-float factors. From |x| = 16 the
 * e^-|x| terms are below 2^-46 and the parts are +-e^|x|/2 cos y and
 * e^|x|/2 sin y through the exp2 core, so a finite part past the overflow
 * of sinhf survives (PLAN D65).
 */
float complex csinhf(float complex z)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	struct __libc_ff sine;
	struct __libc_ff cosine;
	struct __libc_ff sinh;
	struct __libc_ff cosh;
	struct __libc_ff power;

	if (y == 0)
		return CMPLXF(sinhf(x), y);
	if (x == 0 && !isfinite(y))
		return CMPLXF(x, y - y);
	if (isinf(x))
		return isfinite(y) ? CMPLXF(x * cosf(y), INFINITY * sinf(y)) : CMPLXF(x, y - y);
	if (isnan(x))
		return CMPLXF(x, x);
	if (!isfinite(y))
		return CMPLXF(y - y, y - y);

	__libc_sincosf_core((struct __libc_ff){ .hi = y, .lo = 0.0f }, &sine, &cosine);
	if (fabsf(x) >= 16.0f) {
		/* e^|x|/2 = 2^(|x|/ln 2 - 1), |x| clamped where every part overflows */
		power = __libc_ff_scale(fminf(fabsf(x), 200.0f), __LIBC_INV_LN2F);
		power = __libc_ff_add(power, (struct __libc_ff){ .hi = -1.0f, .lo = 0.0f });
		return CMPLXF(copysignf(1.0f, x) * __libc_exp2f_times(power, cosine), __libc_exp2f_times(power, sine));
	}

	__libc_sinhcoshf_core(x, &sinh, &cosh);

	return CMPLXF(__libc_ff_round_product(sinh, cosine, 0), __libc_ff_round_product(cosh, sine, 0));
}
