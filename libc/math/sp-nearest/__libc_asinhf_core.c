/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_asinhf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/*
 * asinh t as float-float for a finite float-float t: below 2^-20 t itself,
 * below 1/8 the odd series of asinhf on t.hi (the cube exact) plus t, up to
 * 2^20 log(|t| + sqrt(t^2 + 1)) through the log2 core, beyond log(2|t|);
 * the sign of t restored (PLAN D62, D67).
 */
struct __libc_ff __libc_asinhf_core(struct __libc_ff t)
{
	struct __libc_ff a = signbit(t.hi) ? (struct __libc_ff){ .hi = -t.hi, .lo = -t.lo } : t;
	struct __libc_ff value;
	struct __libc_ff square;

	if (a.hi < 0x1p-20f)
		return t;

	if (a.hi < 0.125f) {
		square = __libc_ff_product(t.hi, t.hi);
		value = __libc_ff_multiply(__libc_ff_multiply((struct __libc_ff){ .hi = t.hi, .lo = 0.0f }, square), __libc_ff_fast_sum(-0x1.555556p-3f, 0x1.555556p-28f + square.hi * (0x1.333334p-4f - square.hi * (0x1.6db6dcp-5f - square.hi * (0x1.f1c71cp-6f - square.hi * 0x1.6e8ba2p-6f)))));
		return __libc_ff_add(t, value);
	}

	if (a.hi > 0x1p20f) {
		value = __libc_log2f_core(a.hi, a.lo);
		value = __libc_ff_add(value, (struct __libc_ff){ .hi = 1.0f, .lo = 0.0f });
	} else {
		square = __libc_ff_add(__libc_ff_multiply(a, a), (struct __libc_ff){ .hi = 1.0f, .lo = 0.0f });
		value = __libc_ff_add(a, __libc_ff_sqrt(square));
		value = __libc_log2f_core(value.hi, value.lo);
	}
	value = __libc_ff_multiply(value, __LIBC_LN2F);

	return signbit(t.hi) ? (struct __libc_ff){ .hi = -value.hi, .lo = -value.lo } : value;
}
