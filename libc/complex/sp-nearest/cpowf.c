/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cpowf.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.8.2: exp(w log z) in base 2: with L = log2|z| and t = arg z as
 * float-float, the exponent is Re w L - Im w t / ln 2 and the angle
 * Re w t + Im w L ln 2 (an error in a large angle would become the phase
 * error of the result), then the sin core and one scaled rounding per part.
 * w = 0 gives 1, a zero z with a positive real w gives 0; non-finite
 * operands take the plain formula (Annex G cases) (PLAN D65).
 */
float complex cpowf(float complex z, float complex w)
{
	float x = __libc_ref(z);
	float y = __libc_imf(z);
	float u = __libc_ref(w);
	float v = __libc_imf(w);
	float a = fmaxf(fabsf(x), fabsf(y));
	float b = fminf(fabsf(x), fabsf(y));
	struct __libc_ff modulus;
	struct __libc_ff angle;
	struct __libc_ff power;
	struct __libc_ff phase;
	struct __libc_ff sine;
	struct __libc_ff cosine;
	int scale;

	if (u == 0 && v == 0)
		return CMPLXF(1.0f, 0.0f);
	if (x == 0 && y == 0 && v == 0 && u > 0)
		return CMPLXF(0.0f, 0.0f);
	if (!isfinite(x) || !isfinite(y) || !isfinite(u) || !isfinite(v) || a == 0)
		return cexpf(w * clogf(z));

	/* log2|z| = log2(a^2 + b^2)/2 on the parts scaled by 2^-scale, plus scale */
	scale = ilogbf(a);
	a = scalbnf(a, -scale);
	b = scalbnf(b, -scale);
	modulus = __libc_ff_add(__libc_ff_product(a, a), __libc_ff_product(b, b));
	modulus = __libc_log2f_core(modulus.hi, modulus.lo);
	modulus = __libc_ff_add((struct __libc_ff){ .hi = 0.5f * modulus.hi, .lo = 0.5f * modulus.lo }, (struct __libc_ff){ .hi = (float)scale, .lo = 0.0f });
	angle = __libc_atan2f_core((struct __libc_ff){ .hi = scalbnf(y, -scale), .lo = 0.0f }, (struct __libc_ff){ .hi = scalbnf(x, -scale), .lo = 0.0f });

	power = __libc_ff_add(__libc_ff_scale(u, modulus), __libc_ff_multiply(__libc_ff_scale(-v, angle), __LIBC_INV_LN2F));
	phase = __libc_ff_add(__libc_ff_scale(u, angle), __libc_ff_multiply(__libc_ff_scale(v, modulus), __LIBC_LN2F));
	if (!isfinite(power.hi) || !isfinite(phase.hi) || fabsf(phase.hi) >= 0x1p24f)
		return cexpf(w * clogf(z));

	__libc_sincosf_core(phase, &sine, &cosine);

	return CMPLXF(__libc_exp2f_times(power, cosine), __libc_exp2f_times(power, sine));
}
