/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * reference_multiply.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "softdouble.h"
#include "softdp.h"

/*
 * The model of the soft-dp layer's multiplication (PLAN D99), a * b in C
 * for any operands: the 128-bit product of the significands, each moved
 * to the top of 64 bits, from four 32 by 32 to 64-bit products, then one
 * rounding to nearest. x86_64 checks it against the hardware (direct.c);
 * the Arm cores check __aeabi_dmul's assembly against it (assembly.c).
 */
__LIBC_AEABI double reference_multiply(double a, double b)
{
	uint64_t ua = __libc_softdouble_bits(a);
	uint64_t ub = __libc_softdouble_bits(b);
	uint64_t sign = (ua ^ ub) & __LIBC_SOFTDOUBLE_SIGN;
	uint64_t ma;
	uint64_t mb;
	uint64_t high;
	uint32_t sticky;
	int ea;
	int eb;

	if (__libc_softdouble_is_nan(ua))
		return __libc_softdouble_from(ua | __LIBC_SOFTDOUBLE_QUIET);
	if (__libc_softdouble_is_nan(ub))
		return __libc_softdouble_from(ub | __LIBC_SOFTDOUBLE_QUIET);
	if ((ua << 1) == (__LIBC_SOFTDOUBLE_INFINITY << 1) || (ub << 1) == (__LIBC_SOFTDOUBLE_INFINITY << 1)) {
		/* Infinity times zero is invalid (7.2) */
		if ((ua << 1) == 0 || (ub << 1) == 0)
			return __libc_softdouble_from(__LIBC_SOFTDOUBLE_DEFAULT_NAN);
		return __libc_softdouble_from(sign | __LIBC_SOFTDOUBLE_INFINITY);
	}
	if ((ua << 1) == 0 || (ub << 1) == 0)
		return __libc_softdouble_from(sign);

	ma = __libc_softdouble_unpack(ua, &ea) << 11;
	mb = __libc_softdouble_unpack(ub, &eb) << 11;

	/* ma mb = high 2^64 + low; only whether low is zero matters */
	{
		uint32_t a1 = (uint32_t)(ma >> 32);
		uint32_t a0 = (uint32_t)ma;
		uint32_t b1 = (uint32_t)(mb >> 32);
		uint32_t b0 = (uint32_t)mb;
		uint64_t low = (uint64_t)a0 * b0;
		uint64_t cross1 = (uint64_t)a1 * b0;
		uint64_t cross2 = (uint64_t)a0 * b1;
		uint64_t middle = (low >> 32) + (uint32_t)cross1 + (uint32_t)cross2;

		high = (uint64_t)a1 * b1 + (cross1 >> 32) + (cross2 >> 32) + (middle >> 32);
		sticky = ((uint32_t)middle | (uint32_t)low) != 0;
	}

	/* The product lies in [2^126, 2^128): high in [2^62, 2^64), brought to [2^62, 2^63) */
	ea += eb - 1023;
	if ((int64_t)high < 0) {
		sticky |= (uint32_t)high & 1;
		high >>= 1;
		++ea;
	}

	return __libc_softdouble_from(__libc_softdouble_pack(sign, ea, high | sticky));
}
