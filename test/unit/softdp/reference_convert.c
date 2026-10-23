/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * reference_convert.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdbit.h>
#include <stdint.h>

#include "internal/aeabi.h"
#include "internal/softdouble.h"
#include "softdp.h"

/*
 * The models of the soft-dp layer's conversions that exist only in Thumb-2
 * assembly (PLAN D99, D102): __aeabi_f2d, __aeabi_d2iz, __aeabi_d2uiz,
 * __aeabi_l2d and __aeabi_ul2d, as C in 32-bit words where that is how the
 * value splits, and through the layer's own rounding for the 64-bit ones.
 * x86_64 checks them against the hardware (direct.c); the Arm cores check
 * the assembly against them (assembly.c). __aeabi_d2f's model is the
 * layer's general C version, __libc_softdouble_d2f.
 */

/* A normal float moves its field 20 bits down with the significand above it, the bias grown by 1023 - 127 */
uint64_t reference_f2d(uint32_t bits)
{
	uint32_t sign = bits & 0x80000000u;
	uint32_t magnitude = bits & 0x7fffffffu;
	int shift;

	if (magnitude - 0x00800000u < 0x7f000000u)
		return (uint64_t)(sign | ((magnitude >> 3) + ((1023 - 127) << 20))) << 32 | (uint64_t)(bits << 29);
	if (magnitude >= 0x7f800000u) {
		uint64_t result = (uint64_t)(sign | 0x7ff00000u | (magnitude & 0x007fffffu) >> 3) << 32 | (uint64_t)(bits << 29);

		/* A NaN keeps its payload, quietened */
		return magnitude > 0x7f800000u ? result | __LIBC_SOFTDOUBLE_QUIET : result;
	}
	if (magnitude == 0)
		return (uint64_t)sign << 32;

	/* A subnormal: the leading bit to the hidden position, the exponent down to match */
	shift = (int)stdc_leading_zeros(magnitude) - 8;
	return (uint64_t)sign << 32 | (((uint64_t)(1023 - 127 - shift) << 52) + ((uint64_t)(magnitude << shift) << 29));
}

/* The top 32 bits of the significand shifted right 31 - t places; saturating out of range, NaN giving 0 */
int32_t reference_d2iz(uint64_t bits)
{
	uint32_t high = (uint32_t)(bits >> 32);
	uint32_t low = (uint32_t)bits;
	uint32_t shift = 1023 + 31 - ((high >> 20) & 0x7ff);
	uint32_t magnitude;

	if (shift - 1 >= 31) {
		if ((int32_t)shift > 0)
			return 0;
		if (__libc_softdouble_is_nan(bits))
			return 0;
		return (int32_t)high < 0 ? INT32_MIN : INT32_MAX;
	}

	magnitude = ((high << 11) | (low >> 21) | 0x80000000u) >> shift;
	return (int32_t)high < 0 ? (int32_t)(0 - magnitude) : (int32_t)magnitude;
}

/* As reference_d2iz, unsigned: a negative value and NaN give 0 */
uint32_t reference_d2uiz(uint64_t bits)
{
	uint32_t high = (uint32_t)(bits >> 32);
	uint32_t low = (uint32_t)bits;
	uint32_t shift = 1023 + 31 - ((high >> 20) & 0x7ff);

	if ((int32_t)high < 0 || __libc_softdouble_is_nan(bits))
		return 0;
	if (shift >= 32)
		return (int32_t)shift > 0 ? 0 : UINT32_MAX;

	return ((high << 11) | (low >> 21) | 0x80000000u) >> shift;
}

/* The magnitude normalised to bit 63, then the layer's rounding to nearest */
uint64_t reference_ul2d(uint64_t sign, uint64_t u)
{
	unsigned int shift;

	if (u == 0)
		return sign;

	shift = stdc_leading_zeros(u);
	u <<= shift;
	return __libc_softdouble_pack(sign, 1086 - (int)shift, (u >> 1) | (u & 1));
}

uint64_t reference_l2d(int64_t a)
{
	return reference_ul2d(a < 0 ? __LIBC_SOFTDOUBLE_SIGN : 0, a < 0 ? 0 - (uint64_t)a : (uint64_t)a);
}
