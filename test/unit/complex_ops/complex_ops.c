/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * complex_ops.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * The compiler's helpers for complex multiplication and division and for
 * __builtin_powi (PLAN D100): the soft-dp layer's own __muldc3, __divdc3,
 * __divsc3 and __powidf2 on armv7e-m and armv8-m.main, libgcc's elsewhere.
 * Both must meet C23 Annex G.5.2p1's infinity and zero cases, scale a
 * division whose divisor squared would overflow or underflow, and stay
 * within a few ulps of the textbook formula where that is safe. Every
 * operation goes through volatile operands, so it runs at run time.
 */

static int failures;

static void fail(const char *what, int index)
{
	char message[96];

	snprintf(message, sizeof(message), "fail: %s %d\n", what, index);
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

/* A value in (-2^8, 2^8), any significand: products and quotients stay far from the range's ends */
static double moderate(uint64_t *state)
{
	uint64_t r = next(state);
	double x = (double)(r >> 11) * 0x1p-53;

	return ((r & 1) != 0 ? -x : x) * (double)(1 << (r >> 4) % 9);
}

static double complex multiply(double complex z, double complex w)
{
	volatile double complex a = z;
	volatile double complex b = w;

	return a * b;
}

static double complex divide(double complex z, double complex w)
{
	volatile double complex a = z;
	volatile double complex b = w;

	return a / b;
}

static float complex dividef(float complex z, float complex w)
{
	volatile float complex a = z;
	volatile float complex b = w;

	return a / b;
}

static double powi(double x, int n)
{
	volatile double a = x;
	volatile int b = n;

	return __builtin_powi(a, b);
}

/* "An infinity": at least one infinite part (Annex G.3p1) */
static bool infinite(double complex z)
{
	return isinf(creal(z)) || isinf(cimag(z));
}

static bool zero(double complex z)
{
	return creal(z) == 0.0 && cimag(z) == 0.0;
}

/* |x - y| within ulps of y's binade */
static bool close(double x, double y, double ulps)
{
	return fabs(x - y) <= ulps * (y == 0.0 ? 0x1p-1074 : ldexp(DBL_EPSILON, ilogb(y)));
}

static bool closef(float x, float y, float ulps)
{
	return fabsf(x - y) <= ulps * (y == 0.0f ? 0x1p-149f : ldexpf(FLT_EPSILON, ilogbf(y)));
}

static void test_multiply(void)
{
	uint64_t state = 0x9e3779b97f4a7c15ULL;
	double complex z;

	z = multiply(CMPLX(1.0, 2.0), CMPLX(3.0, -1.0));
	if (creal(z) != 5.0 || cimag(z) != 5.0)
		fail("mul exact", 0);

	/* G.5.2p1: an infinity times a nonzero finite number or an infinity is an infinity */
	if (!infinite(multiply(CMPLX(INFINITY, NAN), CMPLX(1.0, 1.0))) || !infinite(multiply(CMPLX(NAN, -INFINITY), CMPLX(0.5, -2.0))))
		fail("mul infinity finite", 0);
	if (!infinite(multiply(CMPLX(1.0, 1.0), CMPLX(NAN, INFINITY))) || !infinite(multiply(CMPLX(INFINITY, INFINITY), CMPLX(INFINITY, -INFINITY))))
		fail("mul infinity infinity", 0);
	if (!infinite(multiply(CMPLX(0x1p1000, 0x1p1000), CMPLX(0x1p1000, -0x1p1000))))
		fail("mul overflow", 0);

	/* A NaN times a finite number stays NaN */
	z = multiply(CMPLX(NAN, 0.0), CMPLX(1.0, 0.0));
	if (!isnan(creal(z)) || infinite(z))
		fail("mul nan", 0);

	/* The textbook formula, bit for bit, where nothing overflows */
	for (int i = 0; i < 2000; ++i) {
		double a = moderate(&state);
		double b = moderate(&state);
		double c = moderate(&state);
		double d = moderate(&state);
		volatile double ac = a * c;
		volatile double bd = b * d;
		volatile double ad = a * d;
		volatile double bc = b * c;

		z = multiply(CMPLX(a, b), CMPLX(c, d));
		if (creal(z) != ac - bd || cimag(z) != ad + bc)
			fail("mul formula", i);
	}
}

static void test_divide(void)
{
	uint64_t state = 0x2545f4914f6cdd1dULL;
	double complex z;
	float complex f;

	z = divide(CMPLX(5.0, 5.0), CMPLX(3.0, -1.0));
	if (!close(creal(z), 1.0, 2) || !close(cimag(z), 2.0, 2))
		fail("div exact", 0);

	/* Divisors whose squares overflow or underflow: the scaling keeps the quotient */
	z = divide(CMPLX(0x1p1000, 0x1p1000), CMPLX(0x1p1000, 0x1p1000));
	if (!close(creal(z), 1.0, 2) || fabs(cimag(z)) > 0x1p-50)
		fail("div large", 0);
	z = divide(CMPLX(0x1p-1000, 0x1p-1000), CMPLX(0x1p-1000, -0x1p-1000));
	if (fabs(creal(z)) > 0x1p-50 || !close(cimag(z), 1.0, 2))
		fail("div small", 0);
	z = divide(CMPLX(0x1p-1000, 0.0), CMPLX(0x1p-1060, 0.0));
	if (creal(z) != 0x1p60 || cimag(z) != 0.0)
		fail("div subnormal divisor", 0);
	if (creal(divide(CMPLX(1.0, 0.0), CMPLX(0x1p-1060, 0.0))) != INFINITY)
		fail("div overflow", 0);

	/* G.5.2p1: over zero an infinity, an infinity over a finite number an infinity, a finite number over an infinity a zero */
	if (!infinite(divide(CMPLX(1.0, 1.0), CMPLX(0.0, 0.0))) || !infinite(divide(CMPLX(-2.0, 0.0), CMPLX(-0.0, 0.0))))
		fail("div by zero", 0);
	if (!infinite(divide(CMPLX(INFINITY, 0.0), CMPLX(1.0, 1.0))) || !infinite(divide(CMPLX(NAN, -INFINITY), CMPLX(3.0, -4.0))))
		fail("div infinity", 0);
	if (!zero(divide(CMPLX(1.0, 1.0), CMPLX(INFINITY, 0.0))) || !zero(divide(CMPLX(-5.0, 2.0), CMPLX(NAN, INFINITY))))
		fail("div by infinity", 0);

	f = dividef(CMPLXF(5.0f, 5.0f), CMPLXF(3.0f, -1.0f));
	if (crealf(f) != 1.0f || cimagf(f) != 2.0f)
		fail("divf exact", 0);
	f = dividef(CMPLXF(0x1p100f, 0x1p100f), CMPLXF(0x1p100f, 0x1p100f));
	if (crealf(f) != 1.0f || cimagf(f) != 0.0f)
		fail("divf large", 0);
	f = dividef(CMPLXF(0x1p-140f, 0.0f), CMPLXF(0x1p-140f, 0x1p-140f));
	if (crealf(f) != 0.5f || cimagf(f) != -0.5f)
		fail("divf subnormal", 0);
	if (!infinite(dividef(CMPLXF(1.0f, 0.0f), CMPLXF(0.0f, 0.0f))) || !infinite(dividef(CMPLXF(INFINITY, 1.0f), CMPLXF(2.0f, 0.0f))) || !zero(dividef(CMPLXF(1.0f, 1.0f), CMPLXF(0.0f, -INFINITY))))
		fail("divf specials", 0);

	/* Within a few ulps of the textbook formula where it cannot overflow */
	for (int i = 0; i < 2000; ++i) {
		double a = moderate(&state);
		double b = moderate(&state);
		double c = moderate(&state) + 0x1p-4;
		double d = moderate(&state);
		volatile double denominator = c * c + d * d;
		volatile double re = (a * c + b * d) / denominator;
		volatile double im = (b * c - a * d) / denominator;

		/* The parts can cancel; compare them against the larger of the two */
		z = divide(CMPLX(a, b), CMPLX(c, d));
		if (!close(creal(z) - re, 0.0, 0) && fabs(creal(z) - re) > 0x1p-48 * fmax(fabs(re), fabs(im)))
			fail("div formula re", i);
		if (!close(cimag(z) - im, 0.0, 0) && fabs(cimag(z) - im) > 0x1p-48 * fmax(fabs(re), fabs(im)))
			fail("div formula im", i);

		f = dividef(CMPLXF((float)a, (float)b), CMPLXF((float)c, (float)d));
		denominator = (double)(float)c * (float)c + (double)(float)d * (float)d;
		re = ((double)(float)a * (float)c + (double)(float)b * (float)d) / denominator;
		im = ((double)(float)b * (float)c - (double)(float)a * (float)d) / denominator;
		if (!closef(crealf(f), (float)re, 2) && fabs(crealf(f) - re) > 0x1p-21 * fmax(fabs(re), fabs(im)))
			fail("divf formula re", i);
		if (!closef(cimagf(f), (float)im, 2) && fabs(cimagf(f) - im) > 0x1p-21 * fmax(fabs(re), fabs(im)))
			fail("divf formula im", i);
	}
}

static void test_powi(void)
{
	if (powi(1.5, 7) != 17.0859375 || powi(-2.0, 3) != -8.0 || powi(2.0, -3) != 0.125 || powi(0.5, -2) != 4.0)
		fail("powi exact", 0);
	if (powi(3.0, 0) != 1.0 || powi(1.0, INT32_MIN) != 1.0 || powi(-1.0, INT32_MAX) != -1.0)
		fail("powi edges", 0);
	if (powi(2.0, 1024) != INFINITY || powi(0x1p-600, 2) != 0.0)
		fail("powi range", 0);
}

int main(void)
{
	test_multiply();
	test_divide();
	test_powi();

	fputs(failures == 0 ? "complex_ops: pass\n" : "complex_ops: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
