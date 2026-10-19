/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * cpow.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * C23 7.3.8.2: exp(w log z), with log z and the product w log z as
 * double-doubles (an error in a large imaginary part would turn into the
 * phase error of the result), then the D47/D48 exp, sin and cos cores and one
 * scaled rounding per part. w = 0 gives 1, a zero z with a positive real w
 * gives 0; non-finite operands take the plain formula (Annex G cases).
 */
double complex cpow(double complex z, double complex w)
{
	double x = __libc_re(z);
	double y = __libc_im(z);
	double a = fmax(fabs(x), fabs(y));
	double b = fmin(fabs(x), fabs(y));
	struct __libc_dd log_modulus;
	struct __libc_dd angle;
	struct __libc_dd real;
	struct __libc_dd imag;
	struct __libc_dd r;
	struct __libc_dd value;
	struct __libc_dd cosine;
	struct __libc_dd sine;
	int scale;
	int exponent;
	int mode;

	if (__libc_re(w) == 0 && __libc_im(w) == 0)
		return CMPLX(1.0, 0.0);
	if (x == 0 && y == 0 && __libc_im(w) == 0 && __libc_re(w) > 0)
		return CMPLX(0.0, 0.0);
	if (!isfinite(x) || !isfinite(y) || !isfinite(__libc_re(w)) || !isfinite(__libc_im(w)) || a == 0)
		return cexp(w * clog(z));

	mode = __libc_math_nearest();
	/* log|z| = log(a^2 + b^2)/2 on the parts scaled by 2^-scale, plus scale ln 2 */
	scale = ilogb(a);
	a = scalbn(a, -scale);
	b = scalbn(b, -scale);
	log_modulus = __libc_dd_add(__libc_dd_product(a, a), __libc_dd_product(b, b));
	log_modulus = __libc_dd_fast_sum(__libc_log_core(log_modulus.hi).hi, __libc_log_core(log_modulus.hi).lo + log_modulus.lo / log_modulus.hi);
	log_modulus = __libc_dd_add((struct __libc_dd){ .hi = 0.5 * log_modulus.hi, .lo = 0.5 * log_modulus.lo }, __libc_dd_multiply(__LIBC_LN2, (struct __libc_dd){ .hi = scale, .lo = 0.0 }));
	angle = __libc_atan2_core((struct __libc_dd){ .hi = scalbn(y, -scale), .lo = 0.0 }, (struct __libc_dd){ .hi = scalbn(x, -scale), .lo = 0.0 });

	/* w log z */
	real = __libc_dd_add(__libc_dd_multiply((struct __libc_dd){ .hi = __libc_re(w), .lo = 0.0 }, log_modulus), __libc_dd_negate(__libc_dd_multiply((struct __libc_dd){ .hi = __libc_im(w), .lo = 0.0 }, angle)));
	imag = __libc_dd_add(__libc_dd_multiply((struct __libc_dd){ .hi = __libc_re(w), .lo = 0.0 }, angle), __libc_dd_multiply((struct __libc_dd){ .hi = __libc_im(w), .lo = 0.0 }, log_modulus));
	if (fabs(real.hi) > 745.0 || !isfinite(imag.hi)) {
		__libc_math_restore(mode);
		return cexp(CMPLX(real.hi, imag.hi));
	}

	switch (__libc_rem_pio2(imag.hi, &r)) {
		case 0:
			r = __libc_dd_add(r, (struct __libc_dd){ .hi = imag.lo, .lo = 0.0 });
			cosine = __libc_cos_core(r);
			sine = __libc_sin_core(r);
			break;
		case 1:
			r = __libc_dd_add(r, (struct __libc_dd){ .hi = imag.lo, .lo = 0.0 });
			cosine = __libc_dd_negate(__libc_sin_core(r));
			sine = __libc_cos_core(r);
			break;
		case 2:
			r = __libc_dd_add(r, (struct __libc_dd){ .hi = imag.lo, .lo = 0.0 });
			cosine = __libc_dd_negate(__libc_cos_core(r));
			sine = __libc_dd_negate(__libc_sin_core(r));
			break;
		default:
			r = __libc_dd_add(r, (struct __libc_dd){ .hi = imag.lo, .lo = 0.0 });
			cosine = __libc_sin_core(r);
			sine = __libc_dd_negate(__libc_cos_core(r));
			break;
	}
	value = __libc_exp_core(real.hi, real.lo, &exponent);
	cosine = __libc_dd_multiply(value, cosine);
	sine = __libc_dd_multiply(value, sine);
	__libc_math_restore(mode);

	return CMPLX(__libc_exp_scale(cosine, exponent), __libc_exp_scale(sine, exponent));
}
