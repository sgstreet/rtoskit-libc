/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * math.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_MATH_H
#define _INTERNAL_MATH_H

#include <fenv.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

/*
 * Helpers of libm (docs/research/fenv-libm.md). Values are taken apart
 * through unions (well defined in C). Exceptions are raised by arithmetic,
 * so on a core without them (D44) nothing happens.
 */

/* The l functions call the double ones (PLAN D7) */
static_assert(LDBL_MANT_DIG == DBL_MANT_DIG && LDBL_MAX_EXP == DBL_MAX_EXP && LDBL_MIN_EXP == DBL_MIN_EXP, "long double must have the format of double");

#define __LIBC_DOUBLE_SIGN 0x8000000000000000ULL
#define __LIBC_DOUBLE_EXPONENT 0x7ff0000000000000ULL
#define __LIBC_DOUBLE_FRACTION 0x000fffffffffffffULL
#define __LIBC_DOUBLE_QUIET 0x0008000000000000ULL
#define __LIBC_DOUBLE_BIAS 1023

#define __LIBC_FLOAT_SIGN 0x80000000U
#define __LIBC_FLOAT_EXPONENT 0x7f800000U
#define __LIBC_FLOAT_FRACTION 0x007fffffU
#define __LIBC_FLOAT_QUIET 0x00400000U
#define __LIBC_FLOAT_BIAS 127

union __libc_double {
	double value;
	uint64_t bits;
};

union __libc_float {
	float value;
	uint32_t bits;
};

static inline uint64_t __libc_double_bits(double x)
{
	return (union __libc_double){ .value = x }.bits;
}

static inline double __libc_double_from(uint64_t bits)
{
	return (union __libc_double){ .bits = bits }.value;
}

static inline uint32_t __libc_float_bits(float x)
{
	return (union __libc_float){ .value = x }.bits;
}

static inline float __libc_float_from(uint32_t bits)
{
	return (union __libc_float){ .bits = bits }.value;
}

/* The biased exponent field */
static inline int __libc_double_exponent(uint64_t bits)
{
	return (int)((bits >> 52) & 0x7ff);
}

static inline int __libc_float_exponent(uint32_t bits)
{
	return (int)((bits >> 23) & 0xff);
}

static inline void __libc_math_raise_invalid(void)
{
	volatile float zero = 0.0f;
	[[maybe_unused]] volatile float result = zero / zero;
}

static inline void __libc_math_raise_divbyzero(void)
{
	volatile float zero = 0.0f;
	[[maybe_unused]] volatile float result = 1.0f / zero;
}

/* Overflow, with inexact as any overflowing operation */
static inline void __libc_math_raise_overflow(void)
{
	volatile float huge = 0x1p127f;
	[[maybe_unused]] volatile float result = huge * huge;
}

/* Underflow, with inexact */
static inline void __libc_math_raise_underflow(void)
{
	volatile float tiny = 0x1p-100f;
	[[maybe_unused]] volatile float result = tiny * tiny;
}

static inline void __libc_math_raise_inexact(void)
{
	volatile float one = 1.0f;
	volatile float tiny = 0x1p-100f;
	[[maybe_unused]] volatile float result = one + tiny;
}

/* A quiet NaN, raising invalid: the result of a domain error (math_errhandling is MATH_ERREXCEPT) */
static inline double __libc_math_invalid(void)
{
	volatile double zero = 0.0;

	return zero / zero;
}

static inline float __libc_math_invalidf(void)
{
	volatile float zero = 0.0f;

	return zero / zero;
}

/* The core of fromfp, ufromfp, fromfpx and ufromfpx */
double __libc_fromfp(double x, int rnd, unsigned int width, bool is_unsigned, bool exact);

/* The remainder core of fmod, remainder and remquo */
double __libc_remainder(double x, double y, bool nearest, int *quotient);

/*
 * An exact value for one final rounding (docs/research/fenv-libm.md):
 * (-1)^negative * (high * 2^64 + low + e) * 2^exponent, where e is 0 when
 * sticky is false and strictly between 0 and 1 otherwise.
 */
struct __libc_exact {
	bool negative;
	bool sticky;
	int exponent;
	uint64_t high;
	uint64_t low;
};

/* The value rounded once in the current direction, with the right exceptions; it must not be zero */
double __libc_exact_to_double(const struct __libc_exact *value);
float __libc_exact_to_float(const struct __libc_exact *value);

/*
 * (-1)^product_negative * mx * my * 2^product_exponent +
 * (-1)^addend_negative * mz * 2^addend_exponent, exactly (mx, my and mz
 * below 2^62, mx and my not zero); false when the sum is exactly zero.
 */
bool __libc_exact_fma(struct __libc_exact *result, bool product_negative, uint64_t mx, uint64_t my, int product_exponent, bool addend_negative, uint64_t mz, int addend_exponent);

/* x = mantissa * 2^*exponent with the mantissa in [2^52, 2^53); x finite and not zero */
static inline uint64_t __libc_double_split(double x, int *exponent)
{
	uint64_t bits = __libc_double_bits(x);
	int biased = __libc_double_exponent(bits);
	uint64_t mantissa = bits & __LIBC_DOUBLE_FRACTION;

	if (biased != 0) {
		mantissa |= 1ULL << 52;
	} else {
		int shift = __builtin_clzll(mantissa) - 11;

		mantissa <<= shift;
		biased = 1 - shift;
	}

	*exponent = biased - __LIBC_DOUBLE_BIAS - 52;
	return mantissa;
}

/* x = mantissa * 2^*exponent with the mantissa in [2^23, 2^24); x finite and not zero */
static inline uint64_t __libc_float_split(float x, int *exponent)
{
	uint32_t bits = __libc_float_bits(x);
	int biased = __libc_float_exponent(bits);
	uint32_t mantissa = bits & __LIBC_FLOAT_FRACTION;

	if (biased != 0) {
		mantissa |= 1U << 23;
	} else {
		int shift = __builtin_clz(mantissa) - 8;

		mantissa <<= shift;
		biased = 1 - shift;
	}

	*exponent = biased - __LIBC_FLOAT_BIAS - 23;
	return mantissa;
}

/*
 * floor(sqrt(high * 2^64 + low)) for a radicand of at most 2 * digits bits
 * (digits at most 60), digit by digit; *inexact tells whether it was not a
 * perfect square.
 */
static inline uint64_t __libc_isqrt(uint64_t high, uint64_t low, int digits, bool *inexact)
{
	uint64_t root = 0;
	uint64_t remainder = 0;

	for (int i = digits - 1; i >= 0; --i) {
		int shift = 2 * i;
		uint64_t pair = shift >= 64 ? (high >> (shift - 64)) & 3 : (low >> shift) & 3;
		uint64_t trial;

		remainder = (remainder << 2) | pair;
		trial = (root << 2) | 1;
		root <<= 1;
		if (remainder >= trial) {
			remainder -= trial;
			root |= 1;
		}
	}

	*inexact = remainder != 0;
	return root;
}

/*
 * Double-double arithmetic for the exponential and logarithm cores
 * (docs/research/libm-exp-log.md): a value hi + lo with |lo| at most half an
 * ulp of hi. The error-free transformations are exact only when rounding to
 * nearest, so the cores select that direction (__libc_math_nearest, D47).
 * Products split their factors (Veltkamp), so they need no fma; the factors
 * must stay below 2^995.
 */
struct __libc_dd {
	double hi;
	double lo;
};

/* a + b exactly, for any a and b */
static inline struct __libc_dd __libc_dd_sum(double a, double b)
{
	double sum = a + b;
	double bb = sum - a;

	return (struct __libc_dd){ .hi = sum, .lo = (a - (sum - bb)) + (b - bb) };
}

/* a + b exactly, when |a| >= |b| (or a is zero) */
static inline struct __libc_dd __libc_dd_fast_sum(double a, double b)
{
	double sum = a + b;

	return (struct __libc_dd){ .hi = sum, .lo = b - (sum - a) };
}

static inline void __libc_dd_split(double a, double *high, double *low)
{
	double c = 0x1.0000002p27 * a;

	*high = c - (c - a);
	*low = a - *high;
}

/* a * b exactly */
static inline struct __libc_dd __libc_dd_product(double a, double b)
{
	double product = a * b;
	double ah;
	double al;
	double bh;
	double bl;

	__libc_dd_split(a, &ah, &al);
	__libc_dd_split(b, &bh, &bl);

	return (struct __libc_dd){ .hi = product, .lo = ((ah * bh - product) + ah * bl + al * bh) + al * bl };
}

/* (a.hi + a.lo) * (b.hi + b.lo), to about 2^-100 */
static inline struct __libc_dd __libc_dd_multiply(struct __libc_dd a, struct __libc_dd b)
{
	struct __libc_dd product = __libc_dd_product(a.hi, b.hi);

	return __libc_dd_fast_sum(product.hi, product.lo + (a.hi * b.lo + a.lo * b.hi));
}

static inline struct __libc_dd __libc_dd_negate(struct __libc_dd a)
{
	return (struct __libc_dd){ .hi = -a.hi, .lo = -a.lo };
}

/* ln 2, ln 10, 1/ln 2, 1/ln 10 as double-double */
#define __LIBC_LN2 ((struct __libc_dd){ .hi = 0x1.62e42fefa39efp-1, .lo = 0x1.abc9e3b39803fp-56 })
#define __LIBC_LN10 ((struct __libc_dd){ .hi = 0x1.26bb1bbb55516p+1, .lo = -0x1.f48ad494ea3e9p-53 })
#define __LIBC_INV_LN2 ((struct __libc_dd){ .hi = 0x1.71547652b82fep+0, .lo = 0x1.777d0ffda0d24p-56 })
#define __LIBC_INV_LN10 ((struct __libc_dd){ .hi = 0x1.bcb7b1526e50ep-2, .lo = 0x1.95355baaafad3p-57 })

/*
 * exp(xh + xl) = 2^*exponent * (result.hi + result.lo), to about 2^-64,
 * for |xh| below 746.
 */
struct __libc_dd __libc_exp_core(double xh, double xl, int *exponent);

/* 2^exponent * (value.hi + value.lo), rounded once in the current direction (twice for subnormal results) */
double __libc_exp_scale(struct __libc_dd value, int exponent);

/* expm1(zh + zl) for finite zh in (-45, 710) */
double __libc_expm1_core(double zh, double zl);

/* log(x) for positive finite x, to about 2^-64 (relative near 1) */
struct __libc_dd __libc_log_core(double x);

/* log(1 + x) for finite x > -1 with |x| at least 2^-54 */
struct __libc_dd __libc_log1p_core(double x);

/* exp(n * (log.hi + log.lo)) for integer n, signed: the core of pown and compoundn */
double __libc_pow_integer(struct __libc_dd log, long long n, bool negative);

/* Exceptional results, raised by arithmetic: +-infinity with overflow, +-0 (or the smallest subnormal) with underflow, +-infinity with divide-by-zero */
static inline double __libc_math_overflow(double sign)
{
	volatile double huge = 0x1p1023;

	return sign * huge * huge;
}

static inline double __libc_math_underflow(double sign)
{
	volatile double tiny = 0x1p-1022;

	return sign * tiny * tiny;
}

static inline double __libc_math_divbyzero(double sign)
{
	volatile double zero = 0.0;

	return sign / zero;
}

/*
 * The double-double algorithms (TwoSum, the exact product, the cores) are
 * exact only when rounding to nearest: the cores switch to it and restore
 * the caller's direction, which then decides the last rounding.
 */
static inline int __libc_math_nearest(void)
{
	int mode = fegetround();

	if (mode != FE_TONEAREST)
		fesetround(FE_TONEAREST);
	return mode;
}

static inline void __libc_math_restore(int mode)
{
	if (mode != FE_TONEAREST)
		fesetround(mode);
}

/*
 * f(x) for a tiny x where f(x) = x + e, 0 < |e| < ulp(x) / 2 and e has the
 * sign of direction: x scaled by 2^100, nudged by 2^-1074 toward f(x) and
 * scaled back, so the current rounding direction picks x or its neighbour
 * and raises inexact; underflow for a subnormal x (tininess before rounding).
 */
static inline double __libc_math_tiny(double x, double direction)
{
	if (x == 0)
		return x;
	if (fabs(x) < 0x1p-1022)
		__libc_math_raise_underflow();

	return (x * 0x1p100 + copysign(0x1p-1074, direction)) * 0x1p-100;
}

/*
 * The float version, for |x| below 2^-30: the nudge of 2^-40 |x| is exact in
 * double and well inside half a float ulp, so the conversion rounds in the
 * current direction and raises inexact, and underflow for a subnormal result.
 */
static inline float __libc_math_tinyf(float x, double direction)
{
	if (x == 0)
		return x;

	return (float)(x + copysign(fabs((double)x) * 0x1p-40, direction));
}

#endif /* _INTERNAL_MATH_H */
