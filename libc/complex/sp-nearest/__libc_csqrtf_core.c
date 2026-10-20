/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_csqrtf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <math.h>

#include "internal/complex.h"
#include "internal/math.h"

/*
 * sqrt(a + ib) as float-float parts for a float-float a and a float b, both
 * finite and below 2^60 in magnitude: Kahan's method, t = sqrt((|a| + |z|)/2)
 * and the other part |b|/(2t); the real part is not negative and the
 * imaginary part has the sign of b. Below 2^-60 the operands are scaled by
 * 2^100 (their squares would underflow) and the parts by 2^-50 (PLAN D67).
 */
void __libc_csqrtf_core(struct __libc_ff a, float b, struct __libc_ff *real, struct __libc_ff *imag)
{
	struct __libc_ff magnitude;
	struct __libc_ff modulus;
	struct __libc_ff t;
	struct __libc_ff other = { .hi = 0.0f, .lo = 0.0f };
	int shift = 0;

	if (fmaxf(fabsf(a.hi), fabsf(b)) < 0x1p-60f) {
		a = (struct __libc_ff){ .hi = scalbnf(a.hi, 100), .lo = scalbnf(a.lo, 100) };
		b = scalbnf(b, 100);
		shift = -50;
	}
	magnitude = signbit(a.hi) ? (struct __libc_ff){ .hi = -a.hi, .lo = -a.lo } : a;
	modulus = __libc_ff_sqrt(__libc_ff_add(__libc_ff_multiply(a, a), __libc_ff_product(b, b)));
	t = __libc_ff_add(magnitude, modulus);
	t = __libc_ff_sqrt((struct __libc_ff){ .hi = 0.5f * t.hi, .lo = 0.5f * t.lo });
	if (t.hi != 0)
		other = __libc_ff_divide((struct __libc_ff){ .hi = fabsf(b), .lo = 0.0f }, (struct __libc_ff){ .hi = 2.0f * t.hi, .lo = 2.0f * t.lo });
	t = (struct __libc_ff){ .hi = scalbnf(t.hi, shift), .lo = scalbnf(t.lo, shift) };
	other = (struct __libc_ff){ .hi = scalbnf(other.hi, shift), .lo = scalbnf(other.lo, shift) };

	if (!signbit(a.hi)) {
		*real = t;
		*imag = signbit(b) ? (struct __libc_ff){ .hi = -other.hi, .lo = -other.lo } : other;
	} else {
		*real = other;
		*imag = signbit(b) ? (struct __libc_ff){ .hi = -t.hi, .lo = -t.lo } : t;
	}
}
