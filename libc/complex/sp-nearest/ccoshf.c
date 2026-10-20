/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ccoshf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.6.4, G.6.2.4: cosh x cos y + i sinh x sin y, the special cases
 * first; each part one rounding of float-float factors, from |x| = 16
 * through the exp2 core as csinhf (PLAN D65).
 */
float complex ccoshf(float complex z)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	struct __libc_ff sine;
	struct __libc_ff cosine;
	struct __libc_ff sinh;
	struct __libc_ff cosh;
	struct __libc_ff power;

	if (y == 0)
		return CMPLXF(coshf(x), isfinite(x) ? sinhf(x) * y : copysignf(0.0f, x) * y);
	if (x == 0 && !isfinite(y))
		return CMPLXF(y - y, x);
	if (isinf(x))
		return isfinite(y) ? CMPLXF(INFINITY * cosf(y), x * sinf(y)) : CMPLXF(x * x, y - y);
	if (isnan(x))
		return CMPLXF(x, x);
	if (!isfinite(y))
		return CMPLXF(y - y, y - y);

	__libc_sincosf_core((struct __libc_ff){ .hi = y, .lo = 0.0f }, &sine, &cosine);
	if (fabsf(x) >= 16.0f) {
		power = __libc_ff_scale(fminf(fabsf(x), 200.0f), __LIBC_INV_LN2F);
		power = __libc_ff_add(power, (struct __libc_ff){ .hi = -1.0f, .lo = 0.0f });
		return CMPLXF(__libc_exp2f_times(power, cosine), copysignf(1.0f, x) * __libc_exp2f_times(power, sine));
	}

	__libc_sinhcoshf_core(x, &sinh, &cosh);

	return CMPLXF(__libc_ff_round_product(cosh, cosine, 0), __libc_ff_round_product(sinh, sine, 0));
}
