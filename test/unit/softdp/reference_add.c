/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * reference_add.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "internal/softdouble.h"
#include "softdp.h"

/*
 * The model of the soft-dp layer's addition (PLAN D99), a + b in C for any
 * operands: the operand of the larger magnitude keeps its significand, the
 * other is shifted to it with the bits shifted out kept as a sticky bit,
 * and the sum or difference is normalised and rounded once, to nearest.
 * x86_64 checks it against the hardware (direct.c); the Arm cores check
 * __aeabi_dadd's assembly against it (assembly.c). Of two NaNs it returns
 * the first, quietened.
 */
__LIBC_AEABI double reference_add(double a, double b)
{
	uint64_t ua = __libc_softdouble_bits(a);
	uint64_t ub = __libc_softdouble_bits(b);
	uint64_t sign;
	uint64_t ma;
	uint64_t mb;
	unsigned int shift;
	int ea;
	int eb;

	/* The NaN, quietened, is the first operand's if both are (IEEE 754-2019 6.2.3) */
	if (__libc_softdouble_is_nan(ua))
		return __libc_softdouble_from(ua | __LIBC_SOFTDOUBLE_QUIET);
	if (__libc_softdouble_is_nan(ub))
		return __libc_softdouble_from(ub | __LIBC_SOFTDOUBLE_QUIET);

	/* Order by magnitude: |a| >= |b| */
	if ((ua << 1) < (ub << 1)) {
		uint64_t swap = ua;

		ua = ub;
		ub = swap;
	}
	sign = ua & __LIBC_SOFTDOUBLE_SIGN;

	if ((ua << 1) == (__LIBC_SOFTDOUBLE_INFINITY << 1)) {
		/* Infinity minus infinity is invalid (7.2) */
		if (ub == (ua ^ __LIBC_SOFTDOUBLE_SIGN))
			return __libc_softdouble_from(__LIBC_SOFTDOUBLE_DEFAULT_NAN);
		return __libc_softdouble_from(ua);
	}
	if ((ub << 1) == 0) {
		/* x + 0 is x; the sum of two zeros is -0 only if both are (6.3) */
		if ((ua << 1) == 0)
			return __libc_softdouble_from(ua & ub);
		return __libc_softdouble_from(ua);
	}

	ma = __libc_softdouble_unpack(ua, &ea) << 9;
	mb = __libc_softdouble_unpack(ub, &eb) << 9;

	/* mb shifted to ma's exponent, the bits below kept as one sticky bit */
	shift = (unsigned int)(ea - eb);
	if (shift >= 63)
		mb = 1;
	else if (shift != 0)
		mb = (mb >> shift) | ((mb << (64 - shift)) != 0);

	if (((ua ^ ub) & __LIBC_SOFTDOUBLE_SIGN) == 0) {
		/* The sum lies in [2^61, 2^63) */
		ma += mb;
		if (ma < (1ULL << 62)) {
			ma <<= 1;
			--ea;
		}
		return __libc_softdouble_from(__libc_softdouble_pack(sign, ea + 1, ma));
	}

	/* An exact zero difference is +0 in round to nearest (6.3) */
	ma -= mb;
	if (ma == 0)
		return __libc_softdouble_from(0);

	/*
	 * Cancellation of more than two bits happens only for exponents at
	 * most one apart, when mb lost no bits; otherwise the sticky bit stays
	 * below the rounding position.
	 */
	shift = stdc_leading_zeros(ma) - 1;
	return __libc_softdouble_from(__libc_softdouble_pack(sign, ea + 1 - (int)shift, ma << shift));
}
