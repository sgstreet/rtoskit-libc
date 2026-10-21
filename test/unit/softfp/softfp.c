/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * softfp.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "internal/math.h"

/*
 * The soft-float helper replacements of P2.05a (PLAN D55): the 64-bit
 * conversions against the compiler's own, the bit-level double to int64_t
 * conversion, subtraction (the nofp layer's __aeabi_dsub and __aeabi_fsub on
 * Armv6-M) with known answers, and the special-case results that moved out
 * of line into libc/fenv.
 */

static int failures;

static void fail(const char *what, uint64_t value)
{
	char message[128];

	snprintf(message, sizeof(message), "fail: %s 0x%016llx\n", what, (unsigned long long)value);
	fputs(message, stdout);
	++failures;
}

/* xorshift64: deterministic test values */
static uint64_t next(uint64_t *state)
{
	*state ^= *state << 13;
	*state ^= *state >> 7;
	*state ^= *state << 17;
	return *state;
}

static uint64_t bits_of(double x)
{
	uint64_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static uint32_t bits_of_float(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

/* The same double, bit for bit (signed zeros apart from each other) */
static bool same(double a, double b)
{
	return bits_of(a) == bits_of(b);
}

static void check_conversions(uint64_t x)
{
	volatile uint64_t u = x;
	volatile int64_t s = (int64_t)x;

	if (!same(__libc_u64_to_double(u), (double)u))
		fail("u64_to_double", x);
	if (!same(__libc_i64_to_double(s), (double)s))
		fail("i64_to_double", x);
}

static void test_conversions(void)
{
	static const uint64_t edges[] = {
		0,
		1,
		0xffffffff,
		0x100000000,
		0x100000001,
		(1ULL << 53) - 1,
		1ULL << 53,
		(1ULL << 53) + 1,
		(1ULL << 53) + 2,
		(1ULL << 53) + 3,
		(1ULL << 54) + 2,
		(1ULL << 54) + 6,
		0x7ffffffffffffdffULL,
		0x7ffffffffffffe00ULL,
		0x7ffffffffffffe01ULL,
		0x7fffffffffffffffULL,
		0x8000000000000000ULL,
		0x8000000000000001ULL,
		0xfffffffffffff800ULL,
		0xfffffffffffffbffULL,
		0xfffffffffffffc00ULL,
		0xffffffffffffffffULL,
		0xfffffffe00000001ULL,
		0xffffffff00000000ULL,
	};
	uint64_t state = 0x9e3779b97f4a7c15ULL;

	for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
		check_conversions(edges[i]);

	/* Random values of every width, so both halves and every rounding position occur */
	for (int i = 0; i < 20000; ++i) {
		uint64_t value = next(&state);

		check_conversions(value >> (i % 64));
		check_conversions(0 - (value >> (i % 64)));
	}

	/* Known answers: ties to even, the carry into the next power of two, the most negative value */
	if (!same(__libc_u64_to_double((1ULL << 53) + 1), 0x1p53))
		fail("u64 tie", (1ULL << 53) + 1);
	if (!same(__libc_u64_to_double((1ULL << 53) + 3), 0x1p53 + 4))
		fail("u64 tie up", (1ULL << 53) + 3);
	if (!same(__libc_u64_to_double(UINT64_MAX), 0x1p64))
		fail("u64 max", UINT64_MAX);
	if (!same(__libc_i64_to_double(INT64_MIN), -0x1p63))
		fail("i64 min", (uint64_t)INT64_MIN);
	if (!same(__libc_i64_to_double(-((1LL << 53) + 1)), -0x1p53))
		fail("i64 tie", (uint64_t) - ((1LL << 53) + 1));
}

static void check_double_to_i64(double x, int64_t expected)
{
	if (__libc_double_to_i64(x) != expected)
		fail("double_to_i64", bits_of(x));
}

static void test_double_to_i64(void)
{
	uint64_t state = 0x0123456789abcdefULL;

	check_double_to_i64(0.0, 0);
	check_double_to_i64(-0.0, 0);
	check_double_to_i64(1.0, 1);
	check_double_to_i64(-1.0, -1);
	check_double_to_i64(0x1p52, 1LL << 52);
	check_double_to_i64(0x1p52 + 1, (1LL << 52) + 1);
	check_double_to_i64(0x1p53, 1LL << 53);
	check_double_to_i64(-0x1p53 - 2, -(1LL << 53) - 2);
	check_double_to_i64(0x1p62, 1LL << 62);
	check_double_to_i64(0x1.fffffffffffffp62, 0x7ffffffffffffc00LL);
	check_double_to_i64(-0x1p63, INT64_MIN);
	check_double_to_i64(4503599627370495.0, 4503599627370495LL);

	/* Every integral double below 2^63 in magnitude, against the compiler's conversion */
	for (int i = 0; i < 20000; ++i) {
		int64_t value = (int64_t)(next(&state) >> (1 + i % 63));
		volatile double x = (double)(i & 1 ? -value : value);
		int64_t expected = (int64_t)x;

		check_double_to_i64(x, expected);
	}
}

/* x - y through volatile operands, so the subtraction happens at run time (the helper on soft-float cores) */
static double subtract(double x, double y)
{
	volatile double a = x;
	volatile double b = y;

	return a - b;
}

static float subtractf(float x, float y)
{
	volatile float a = x;
	volatile float b = y;

	return a - b;
}

static void test_subtraction(void)
{
	uint64_t state = 0xfeedfacecafebeefULL;

	/* IEEE 754-2019 6.3: an exact zero difference is +0 in round to nearest; -0 - +0 is -0 */
	if (!same(subtract(1.0, 1.0), 0.0) || !same(subtract(0.0, 0.0), 0.0) || !same(subtract(-0.0, 0.0), -0.0) || !same(subtract(0.0, -0.0), 0.0) || !same(subtract(-0.0, -0.0), 0.0))
		fail("sub zeros", 0);
	if (!same(subtract(0x1p-1074, 0x1p-1074), 0.0) || !same(subtract(-2.5, -2.5), 0.0))
		fail("sub exact zero", 0);

	/* Rounding: a tie to even, an exact result one ulp below 1, the subnormal range, cancellation */
	if (!same(subtract(1.0, 0x1p-54), 1.0) || !same(subtract(1.0, 0x1p-53), 0x1.fffffffffffffp-1) || !same(subtract(1.0, 0x1.8p-53), 0x1.ffffffffffffep-1))
		fail("sub rounding", 0);
	if (!same(subtract(0x1p-1022, 0x1p-1074), 0x0.fffffffffffffp-1022) || !same(subtract(0x1.0000000000001p0, 1.0), 0x1p-52))
		fail("sub subnormal", 0);
	if (!same(subtract(0x1.fffffffffffffp1023, -0x1p970), INFINITY) || !same(subtract(-0x1.fffffffffffffp1023, 0x1p970), -INFINITY))
		fail("sub overflow", 0);

	/* Infinities and NaN */
	if (!isnan(subtract(INFINITY, INFINITY)) || !same(subtract(INFINITY, -INFINITY), INFINITY) || !same(subtract(1.0, INFINITY), -INFINITY))
		fail("sub infinity", 0);
	if (!isnan(subtract(NAN, 1.0)) || !isnan(subtract(1.0, NAN)))
		fail("sub nan", 0);

	/* Integers below 2^52 subtract exactly */
	for (int i = 0; i < 20000; ++i) {
		int64_t a = (int64_t)(next(&state) >> 12);
		int64_t b = (int64_t)(next(&state) >> 12);

		if (!same(subtract((double)a, (double)b), (double)(a - b)))
			fail("sub integers", (uint64_t)a);
	}

	if (bits_of_float(subtractf(1.0f, 1.0f)) != bits_of_float(0.0f) || bits_of_float(subtractf(-0.0f, 0.0f)) != bits_of_float(-0.0f))
		fail("subf zeros", 0);
	if (subtractf(1.0f, 0x1p-25f) != 1.0f || subtractf(1.0f, 0x1p-24f) != 0x1.fffffep-1f || subtractf(3.0f, 5.0f) != -2.0f)
		fail("subf rounding", 0);
	if (!isnan(subtractf(INFINITY, INFINITY)) || subtractf(-INFINITY, 1.0f) != -INFINITY)
		fail("subf infinity", 0);
}

static void test_special_results(void)
{
	if (!isnan(__libc_math_invalid()) || !isnan(__libc_math_invalidf()))
		fail("invalid", 0);
	if (!same(__libc_math_divbyzero(1.0), INFINITY) || !same(__libc_math_divbyzero(-1.0), -INFINITY))
		fail("divbyzero", 0);
	if (!same(__libc_math_underflow(1.0), 0.0) || !same(__libc_math_underflow(-1.0), -0.0))
		fail("underflow", 0);
	if (!same(__libc_math_overflow(1.0), INFINITY) || !same(__libc_math_overflow(-1.0), -INFINITY))
		fail("overflow", 0);

	/* Through the public functions that return them */
	if (!same(log(0.0), -INFINITY) || !isnan(log(-1.0)) || !same(exp(-1000.0), 0.0) || !same(atanh(-1.0), -INFINITY))
		fail("public special results", 0);
}

/*
 * The exact products (PLAN D82, D83): the fma form on the FPUs that have
 * one, Dekker's elsewhere. Each board's form must equal Dekker's, written
 * out here, for factors whose product and error term stay normal; the
 * float pair is also checked against double, which holds it exactly.
 */
static void dekker_float(float a, float b, float *high, float *low)
{
	float product = a * b;
	float ca = 0x1.002p12f * a;
	float cb = 0x1.002p12f * b;
	float ah = ca - (ca - a);
	float bh = cb - (cb - b);
	float al = a - ah;
	float bl = b - bh;

	*high = product;
	*low = ((ah * bh - product) + ah * bl + al * bh) + al * bl;
}

static void dekker_double(double a, double b, double *high, double *low)
{
	double product = a * b;
	double ca = 0x1.0000002p27 * a;
	double cb = 0x1.0000002p27 * b;
	double ah = ca - (ca - a);
	double bh = cb - (cb - b);
	double al = a - ah;
	double bl = b - bh;

	*high = product;
	*low = ((ah * bh - product) + ah * bl + al * bh) + al * bl;
}

static void test_exact_products(void)
{
	uint64_t state = 0x5851f42d4c957f2dULL;

	for (int i = 0; i < 20000; ++i) {
		uint64_t r = next(&state);
		uint64_t q = next(&state);
		float a = __libc_float_from(((uint32_t)r & 0x807fffff) | (uint32_t)(97 + (r >> 32) % 60) << 23);
		float b = __libc_float_from(((uint32_t)(r >> 23) & 0x807fffff) | (uint32_t)(97 + (r >> 48) % 60) << 23);
		double c = __libc_double_from((q & 0x800fffffffffffffULL) | (uint64_t)(1023 - 200 + (q >> 52) % 400) << 52);
		double d = __libc_double_from(((q >> 7) & 0x800fffffffffffffULL) | (uint64_t)(1023 - 200 + (r >> 20) % 400) << 52);
		struct __libc_ff pf = __libc_ff_product(a, b);
		struct __libc_dd pd = __libc_dd_product(c, d);
		float fh;
		float fl;
		double dh;
		double dl;

		dekker_float(a, b, &fh, &fl);
		dekker_double(c, d, &dh, &dl);
		if (bits_of_float(pf.hi) != bits_of_float(fh) || bits_of_float(pf.lo) != bits_of_float(fl) || !same((double)pf.hi + (double)pf.lo, (double)a * (double)b))
			fail("ff product", bits_of_float(a) | (uint64_t)bits_of_float(b) << 32);
		if (!same(pd.hi, dh) || !same(pd.lo, dl))
			fail("dd product", bits_of(c));
	}
}

int main(void)
{
	test_conversions();
	test_double_to_i64();
	test_subtraction();
	test_special_results();
	test_exact_products();

	fputs(failures == 0 ? "softfp: pass\n" : "softfp: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
