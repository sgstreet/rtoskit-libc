/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_ddiv.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/aeabi.h"
#include "softdouble.h"

/*
 * a / b in integers (IEEE 754-2019 5.4.1, PLAN D99): long division of the
 * significands in base 2^18, three digits after a leading 1, each estimated
 * from a 32-bit reciprocal of the divisor and corrected by the remainder,
 * which ends exact, so the sticky bit is exact too.
 */
__LIBC_AEABI double __aeabi_ddiv(double a, double b)
{
	uint64_t ua = __libc_softdouble_bits(a);
	uint64_t ub = __libc_softdouble_bits(b);
	uint64_t sign = (ua ^ ub) & __LIBC_SOFTDOUBLE_SIGN;
	uint64_t ma;
	uint64_t mb;
	uint64_t quotient;
	uint64_t remainder;
	uint32_t reciprocal;
	int ea;
	int eb;

	if (__libc_softdouble_is_nan(ua))
		return __libc_softdouble_from(ua | __LIBC_SOFTDOUBLE_QUIET);
	if (__libc_softdouble_is_nan(ub))
		return __libc_softdouble_from(ub | __LIBC_SOFTDOUBLE_QUIET);
	if ((ua << 1) == (__LIBC_SOFTDOUBLE_INFINITY << 1)) {
		/* Infinity over infinity is invalid (7.2) */
		if ((ub << 1) == (__LIBC_SOFTDOUBLE_INFINITY << 1))
			return __libc_softdouble_from(__LIBC_SOFTDOUBLE_DEFAULT_NAN);
		return __libc_softdouble_from(sign | __LIBC_SOFTDOUBLE_INFINITY);
	}
	if ((ub << 1) == (__LIBC_SOFTDOUBLE_INFINITY << 1))
		return __libc_softdouble_from(sign);
	if ((ub << 1) == 0) {
		/* Zero over zero is invalid (7.2); anything else over zero is an exact infinity (7.3) */
		if ((ua << 1) == 0)
			return __libc_softdouble_from(__LIBC_SOFTDOUBLE_DEFAULT_NAN);
		return __libc_softdouble_from(sign | __LIBC_SOFTDOUBLE_INFINITY);
	}
	if ((ua << 1) == 0)
		return __libc_softdouble_from(sign);

	ma = __libc_softdouble_unpack(ua, &ea);
	mb = __libc_softdouble_unpack(ub, &eb);

	/* Start from ma in [mb, 2 mb), so the quotient's leading digit is 1 */
	ea -= eb - 1023;
	if (ma < mb) {
		ma <<= 1;
		--ea;
	}

	/*
	 * reciprocal is below 2^62 / (mb / 2^22 + 1), within 2^-20 of it: the
	 * numerator is 2^62 (1 - 2^-21), so the two roundings of the float
	 * division cannot take it above. Each digit, the remainder times 2^18
	 * over mb, is then underestimated by at most 2, and the remainder,
	 * computed modulo 2^64 but below 3 mb in fact, is exact.
	 */
	reciprocal = (uint32_t)(0x1.fffffcp61f / (float)((uint32_t)(mb >> 22) + 1));
	quotient = 1;
	remainder = ma - mb;
	for (int i = 0; i < 3; ++i) {
		uint32_t digit = (uint32_t)(((uint64_t)(uint32_t)(remainder >> 22) * reciprocal) >> 44);

		remainder = (remainder << 18) - digit * mb;
		while (remainder >= mb) {
			remainder -= mb;
			++digit;
		}
		quotient = (quotient << 18) | digit;
	}

	/* quotient lies in [2^54, 2^55): 53 bits, two more and the remainder as the sticky bit */
	return __libc_softdouble_from(__libc_softdouble_pack(sign, ea, (quotient << 8) | (remainder != 0)));
}
