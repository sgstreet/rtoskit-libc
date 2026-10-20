/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * float_exact.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <complex.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * The exact float functions (P2.05b, PLAN D56) against the double functions
 * on the widened argument: every result is exactly representable, so the
 * float function must return the same bits (NaNs only need to be NaNs). On
 * the single-precision cores the float functions come from the float-only
 * layer (libc/math/sp); elsewhere the test still pins the generic ones.
 * The arguments cover every exponent with random fractions, the boundaries
 * and the special values.
 */

static int failures;

static void fail(const char *what, float x, float y)
{
	char message[160];
	uint32_t bx;
	uint32_t by;

	memcpy(&bx, &x, sizeof(bx));
	memcpy(&by, &y, sizeof(by));
	snprintf(message, sizeof(message), "fail: %s x=0x%08lx y=0x%08lx\n", what, (unsigned long)bx, (unsigned long)by);
	fputs(message, stdout);
	++failures;
}

static uint32_t next(uint32_t *state)
{
	*state ^= *state << 13;
	*state ^= *state >> 17;
	*state ^= *state << 5;
	return *state;
}

static float from_bits(uint32_t bits)
{
	float x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

static bool same(float a, float b)
{
	uint32_t ba;
	uint32_t bb;

	if (isnan(a) || isnan(b))
		return isnan(a) && isnan(b);
	memcpy(&ba, &a, sizeof(ba));
	memcpy(&bb, &b, sizeof(bb));
	return ba == bb;
}

/* The test arguments: special values, the integer and half-integer boundaries, and random fractions at every exponent */
static float argument(int i)
{
	static const float specials[] = { 0.0f, -0.0f, INFINITY, -INFINITY, NAN, 0x1p-149f, -0x1p-149f, 0x1p-126f, 0x1.fffffcp-127f, 0.5f, -0.5f, 1.5f, -1.5f, 2.5f, -2.5f, 0x1.fffffep-2f, 0x1.000002p-1f, 0x1p23f, 0x1.fffffep22f, 0x1.fffffep23f, -0x1p23f, 0x1p31f, -0x1p31f, 0x1.fffffep30f, 0x1p63f, -0x1p63f, 0x1.fffffep62f, 0x1.fffffep127f, -0x1.fffffep127f, 8388607.5f, -8388607.5f, 4194303.5f };
	static uint32_t state = 0x12345678;
	uint32_t random;

	if (i < (int)(sizeof(specials) / sizeof(specials[0])))
		return specials[i];

	random = next(&state);
	/* The exponent cycles through every value, so each rounding position occurs */
	return from_bits((random & 0x807fffff) | ((uint32_t)(i % 255) << 23));
}

static const int count = 40000;

static void test_rounding(void)
{
	for (int i = 0; i < count; ++i) {
		float x = argument(i);

		if (!same(ceilf(x), (float)ceil((double)x)))
			fail("ceilf", x, 0);
		if (!same(floorf(x), (float)floor((double)x)))
			fail("floorf", x, 0);
		if (!same(truncf(x), (float)trunc((double)x)))
			fail("truncf", x, 0);
		if (!same(roundf(x), (float)round((double)x)))
			fail("roundf", x, 0);
		if (!same(roundevenf(x), (float)roundeven((double)x)))
			fail("roundevenf", x, 0);
		if (!same(rintf(x), (float)rint((double)x)))
			fail("rintf", x, 0);
		if (!same(nearbyintf(x), (float)nearbyint((double)x)))
			fail("nearbyintf", x, 0);
	}
}

static void test_conversions(void)
{
	for (int i = 0; i < count; ++i) {
		float x = argument(i);
		double d = x;

		/* Out of range the value is unspecified: the library returns the minimum on every path */
		if (lrintf(x) != lrint(d))
			fail("lrintf", x, 0);
		if (lroundf(x) != lround(d))
			fail("lroundf", x, 0);
		if (llrintf(x) != llrint(d))
			fail("llrintf", x, 0);
		if (llroundf(x) != llround(d))
			fail("llroundf", x, 0);

		for (int rnd = FP_INT_UPWARD; rnd <= FP_INT_TONEARESTFROMZERO + 1; ++rnd) {
			static const unsigned int widths[] = { 0, 1, 2, 8, 24, 31, 32, 33, 63, 64, 65, 127, 128, 129, UINT_MAX };

			for (size_t w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w) {
				unsigned int width = widths[w];

				if (!same(fromfpf(x, rnd, width), (float)fromfp(d, rnd, width)) || !same(ufromfpf(x, rnd, width), (float)ufromfp(d, rnd, width)))
					fail("fromfpf", x, (float)width);
				if (!same(fromfpxf(x, rnd, width), (float)fromfpx(d, rnd, width)) || !same(ufromfpxf(x, rnd, width), (float)ufromfpx(d, rnd, width)))
					fail("fromfpxf", x, (float)width);
			}
		}
	}
}

static void test_decomposition(void)
{
	for (int i = 0; i < count; ++i) {
		float x = argument(i);
		double d = x;
		int exponent_f;
		int exponent_d;
		float integral_f;
		double integral_d;

		if (!same(frexpf(x, &exponent_f), (float)frexp(d, &exponent_d)) || (isfinite(x) && exponent_f != exponent_d))
			fail("frexpf", x, 0);
		if (ilogbf(x) != ilogb(d))
			fail("ilogbf", x, 0);
		if (llogbf(x) != llogb(d))
			fail("llogbf", x, 0);
		if (!same(logbf(x), (float)logb(d)))
			fail("logbf", x, 0);
		if (!same(modff(x, &integral_f), (float)modf(d, &integral_d)) || !same(integral_f, (float)integral_d))
			fail("modff", x, 0);
	}
}

static void test_remainders(void)
{
	static uint32_t state = 0xdeadbeef;

	for (int i = 0; i < count; ++i) {
		float x = argument(i);
		/* Divisors near x as well as anywhere, so long and short divisions both occur */
		float y = (i & 1) != 0 ? argument((int)(next(&state) % (uint32_t)count)) : from_bits((next(&state) & 0x80ffffff) | (((uint32_t)(i % 255) + (next(&state) % 16) - 8) & 0xff) << 23);
		int quotient_f;
		int quotient_d;

		if (!same(fmodf(x, y), (float)fmod(x, y)))
			fail("fmodf", x, y);
		if (!same(remainderf(x, y), (float)remainder(x, y)))
			fail("remainderf", x, y);
		if (!same(remquof(x, y, &quotient_f), (float)remquo(x, y, &quotient_d)) || quotient_f != quotient_d)
			fail("remquof", x, y);
	}
}

/* Quotients exactly halfway between two integers: remainder rounds them to the even one */
static void test_remainder_ties(void)
{
	static uint32_t state = 0x5eed;

	if (remainderf(2.5f, 1.0f) != 0.5f || remainderf(3.5f, 1.0f) != -0.5f || remainderf(5.0f, 2.0f) != 1.0f || remainderf(7.0f, 2.0f) != -1.0f || remainderf(-7.0f, 2.0f) != 1.0f)
		fail("remainderf ties", 0, 0);

	for (int i = 0; i < count; ++i) {
		/* y even, so x = (k + 1/2) y is an integer below 2^24: exact */
		float y = (float)(2 * (next(&state) % 0x40000 + 1));
		float x = (float)(next(&state) % 32) * y + y / 2;
		int quotient_f;
		int quotient_d;

		if (!same(remainderf(x, y), (float)remainder(x, y)) || !same(remainderf(-x, y), (float)remainder(-x, y)))
			fail("remainderf tie", x, y);
		if (!same(remquof(x, -y, &quotient_f), (float)remquo(x, -y, &quotient_d)) || quotient_f != quotient_d)
			fail("remquof tie", x, y);
	}
}

static void test_nexttoward(void)
{
	static uint32_t state = 0xc0ffee;

	for (int i = 0; i < count; ++i) {
		float x = argument(i);
		long double y;
		float expected;

		/* y at x, just above or below it (between two floats), or anywhere */
		switch (next(&state) % 4) {
			case 0:
				y = x;
				break;
			case 1:
				y = (long double)x + (long double)x * 0x1p-30L;
				break;
			case 2:
				y = (long double)x - 0x1p-1000L;
				break;
			default:
				y = argument((int)(next(&state) % (uint32_t)count));
				break;
		}

		if (isnan(x) || isnan(y))
			expected = NAN;
		else if ((long double)x == y)
			expected = (float)y;
		else
			expected = nextafterf(x, (long double)x < y ? INFINITY : -INFINITY);

		if (!same(nexttowardf(x, y), expected))
			fail("nexttowardf", x, (float)y);
	}

	/* The signed zeros: y's sign wins when x equals y */
	if (!same(nexttowardf(0.0f, -0.0L), -0.0f) || !same(nexttowardf(-0.0f, 0.0L), 0.0f) || !same(nexttowardf(0.0f, -1.0L), -0x1p-149f))
		fail("nexttowardf zeros", 0, 0);
	if (!same(nexttowardf(0x1.fffffep127f, INFINITY), INFINITY) || !same(nexttowardf(0x1p-149f, 0.0L), 0.0f))
		fail("nexttowardf limits", 0, 0);
}

static void test_complex(void)
{
	static const float parts[] = { 0.0f, -0.0f, 1.5f, -2.0f, INFINITY, -INFINITY, NAN, 0x1p-149f };

	for (size_t i = 0; i < sizeof(parts) / sizeof(parts[0]); ++i) {
		for (size_t j = 0; j < sizeof(parts) / sizeof(parts[0]); ++j) {
			float complex z = CMPLXF(parts[i], parts[j]);
			double complex w = CMPLX(parts[i], parts[j]);

			if (!same(crealf(z), parts[i]) || !same(cimagf(z), parts[j]))
				fail("crealf/cimagf", parts[i], parts[j]);
			if (!same(crealf(conjf(z)), parts[i]) || !same(cimagf(conjf(z)), -parts[j]))
				fail("conjf", parts[i], parts[j]);
			if (!same(crealf(cprojf(z)), (float)creal(cproj(w))) || !same(cimagf(cprojf(z)), (float)cimag(cproj(w))))
				fail("cprojf", parts[i], parts[j]);
		}
	}
}

int main(void)
{
	test_rounding();
	test_conversions();
	test_decomposition();
	test_remainders();
	test_remainder_ties();
	test_nexttoward();
	test_complex();

	fputs(failures == 0 ? "float_exact: pass\n" : "float_exact: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
