/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * softdouble.h
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_SOFTDOUBLE_H
#define _INTERNAL_SOFTDOUBLE_H

#include <stdbit.h>
#include <stdint.h>

#include "internal/aeabi.h"

/*
 * Double-precision arithmetic in integers for the cores whose FPU has
 * single precision only, or that have no FPU (the soft-dp, soft-int and
 * nofp layers; PLAN D99, D102, docs/research/softdouble.md): round to
 * nearest, ties to even, the only direction <fenv.h> has there (D44), and
 * no exception flags. Nothing in here may compute in double, or the helpers
 * would call themselves.
 */

#define __LIBC_SOFTDOUBLE_SIGN 0x8000000000000000ULL
#define __LIBC_SOFTDOUBLE_INFINITY 0x7ff0000000000000ULL
#define __LIBC_SOFTDOUBLE_QUIET 0x0008000000000000ULL
#define __LIBC_SOFTDOUBLE_DEFAULT_NAN 0x7ff8000000000000ULL
#define __LIBC_SOFTDOUBLE_HIDDEN 0x0010000000000000ULL

/*
 * The compiler's own helpers whose libgcc versions call the double helpers
 * from inside libgcc, where the linker would take libgcc's double objects
 * to resolve them (PLAN D100); their prototypes are the compiler's
 */
_Complex double __muldc3(double a, double b, double c, double d);
_Complex double __divdc3(double a, double b, double c, double d);
_Complex float __divsc3(float a, float b, float c, float d);
double __powidf2(double x, int n);

/* The general cases of __aeabi_d2f, whose fast path is Thumb-2 assembly */
__LIBC_AEABI float __libc_softdouble_d2f(double a);

static inline uint64_t __libc_softdouble_bits(double x)
{
	uint64_t bits;

	__builtin_memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static inline double __libc_softdouble_from(uint64_t bits)
{
	double x;

	__builtin_memcpy(&x, &bits, sizeof(x));
	return x;
}

static inline uint32_t __libc_softdouble_float_bits(float x)
{
	uint32_t bits;

	__builtin_memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static inline float __libc_softdouble_float_from(uint32_t bits)
{
	float x;

	__builtin_memcpy(&x, &bits, sizeof(x));
	return x;
}

/* A NaN (the magnitude above the infinity's), tested on the bits */
static inline bool __libc_softdouble_is_nan(uint64_t bits)
{
	return (bits << 1) > (__LIBC_SOFTDOUBLE_INFINITY << 1);
}

/*
 * The 53-bit significand of a finite nonzero operand with its biased
 * exponent, a subnormal one normalised (its exponent then 0 or below): the
 * value is significand 2^(exponent - 1075), with the significand in
 * [2^52, 2^53).
 */
static inline uint64_t __libc_softdouble_unpack(uint64_t bits, int *exponent)
{
	uint64_t significand = bits & (__LIBC_SOFTDOUBLE_HIDDEN - 1);
	int field = (int)((bits >> 52) & 0x7ff);

	if (field != 0) {
		*exponent = field;
		return significand | __LIBC_SOFTDOUBLE_HIDDEN;
	}

	int shift = (int)stdc_leading_zeros(significand) - 11;

	*exponent = 1 - shift;
	return significand << shift;
}

/*
 * The double nearest x 2^(exponent - 1085), ties to even, with the sign:
 * x lies in [2^62, 2^63), the significand above ten more bits, of which
 * anything nonzero below the first must have reached x (a sticky bit). A
 * result below the normal range is shifted first, so it is rounded only
 * once, on the subnormal grid; a carry out of the significand moves to the
 * next exponent through the addition, to infinity at the top.
 */
uint64_t __libc_softdouble_pack(uint64_t sign, int exponent, uint64_t x);

/* sign | u exactly (C23 6.3.1.4p2: every 32-bit integer is a double) */
static inline uint64_t __libc_softdouble_from_u32(uint64_t sign, uint32_t u)
{
	unsigned int shift;

	if (u == 0)
		return sign;

	shift = stdc_leading_zeros(u);
	return sign | (((uint64_t)(1053 - shift) << 52) + ((uint64_t)(u << shift) << 21));
}

/*
 * The integer part of a finite x (C23 6.3.1.4p1, truncation toward zero) as
 * a magnitude, for |x| below 2^64; larger values and NaN are the caller's.
 */
static inline uint64_t __libc_softdouble_truncate(uint64_t bits)
{
	int field = (int)((bits >> 52) & 0x7ff);
	uint64_t significand = (bits & (__LIBC_SOFTDOUBLE_HIDDEN - 1)) | __LIBC_SOFTDOUBLE_HIDDEN;

	if (field < 1023)
		return 0;
	if (field >= 1075)
		return significand << (field - 1075);

	return significand >> (1075 - field);
}

/*
 * The integer part of a finite float's magnitude (C23 6.3.1.4p1, truncation
 * toward zero), for |x| below 2^64; larger values and NaN are the caller's.
 */
static inline uint64_t __libc_softdouble_truncate_float(uint32_t bits)
{
	int field = (int)((bits >> 23) & 0xff);
	uint64_t significand = (bits & 0x007fffffu) | 0x00800000u;

	if (field < 127)
		return 0;
	if (field >= 150)
		return significand << (field - 150);

	return significand >> (150 - field);
}

#endif /* _INTERNAL_SOFTDOUBLE_H */
