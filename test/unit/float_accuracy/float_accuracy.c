/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * float_accuracy.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * The float transcendental functions against the double functions on the
 * widened argument (good to about 2^-53, far below a float ulp): the error of
 * each result in float ulps of the exact value must stay below 1 (faithful,
 * PLAN D57), and at most one result in a thousand may differ from the
 * correctly rounded one (the reference's own double rounding included). The special values must match exactly. On
 * x86_64 the test links the float-float layer of the single-precision cores
 * (libc/math/sp-nearest) and runs FLOAT_ACCURACY_SAMPLES arguments per
 * function natively; on the Arm boards it measures the library as built.
 */

static int failures;

struct unary {
	const char *name;
	float (*function)(float);
	double (*reference)(double);
	float low;
	float high;
	/* Every float in [sweep_low, sweep_high) is checked where the samples are many: the subnormal results */
	float sweep_low;
	float sweep_high;
};

static const struct unary unary[] = {
	{ "expf", expf, exp, -104.0f, 89.0f, -104.0f, -87.0f },
	{ "exp2f", exp2f, exp2, -151.0f, 128.0f, -151.0f, -126.0f },
	{ "exp10f", exp10f, exp10, -46.0f, 39.0f, -46.0f, -37.5f },
	{ "expm1f", expm1f, expm1, -18.0f, 89.0f },
	{ "exp2m1f", exp2m1f, exp2m1, -26.0f, 128.0f },
	{ "exp10m1f", exp10m1f, exp10m1, -8.0f, 39.0f },
};

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

static uint32_t bits_of(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

/* |f - exact| in ulps of the exact value's binade (subnormal ulp 2^-149); a result past FLT_MAX counts as 2^128 */
static double ulp_error(float f, double exact)
{
	double value = isinf(f) ? copysign(0x1p128, f) : f;
	double magnitude = fabs(exact);
	int exponent;

	if (isnan(exact))
		return isnan(f) ? 0.0 : INFINITY;
	if (isinf(exact))
		return f == exact ? 0.0 : INFINITY;
	if (isnan(f) || (f != 0 && exact != 0 && !signbit(f) != !signbit(exact)))
		return INFINITY;
	/* Past FLT_MAX a faithful result is FLT_MAX or infinity */
	if (magnitude > FLT_MAX)
		return (isinf(f) || fabsf(f) == FLT_MAX) && !signbit(f) == !signbit(exact) ? 0.0 : INFINITY;
	frexp(magnitude, &exponent);
	if (exponent < -125)
		exponent = -125;

	return fabs(value - exact) / ldexp(1.0, exponent - 24);
}

/* Random floats in [low, high]: random bit patterns (every binade) and uniform values, alternately */
static float sample(uint32_t *state, float low, float high, int i)
{
	float x;

	if ((i & 1) != 0)
		return low + (high - low) * ((float)(next(state) >> 8) * 0x1p-24f);
	do {
		x = from_bits(next(state));
	} while (!(x >= low && x <= high));

	return x;
}

static void check_specials(const struct unary *test)
{
	static const float specials[] = { 0.0f, -0.0f, INFINITY, -INFINITY, NAN, 1.0f, -1.0f, 0x1p-149f, -0x1p-149f, 0x1p-126f, -0x1p-126f, 0x1p-30f, -0x1p-30f };

	for (size_t i = 0; i < sizeof(specials) / sizeof(specials[0]); ++i) {
		float x = specials[i];
		float got = test->function(x);
		float expected = (float)test->reference(x);
		char message[128];

		if (isnan(got) && isnan(expected))
			continue;
		if (bits_of(got) == bits_of(expected) || ulp_error(got, test->reference(x)) < 1.0)
			continue;
		snprintf(message, sizeof(message), "fail: %s(%a) = %a, expected %a\n", test->name, (double)x, (double)got, (double)expected);
		fputs(message, stdout);
		++failures;
	}
}

static void check_unary(const struct unary *test)
{
	uint32_t state = 0x2545f491;
	double worst = 0;
	float worst_x = 0;
	long misrounded = 0;
	char message[160];

	long count = FLOAT_ACCURACY_SAMPLES;
	long sweep = 0;
	float swept = test->sweep_low;

	check_specials(test);
	/* Where the platform runs millions of arguments, the sweep adds every float of the subnormal range */
	if (FLOAT_ACCURACY_SAMPLES >= 0x400000 && test->sweep_low < test->sweep_high)
		sweep = 1;
	for (long i = 0; i < count || (sweep != 0 && swept < test->sweep_high); ++i) {
		float x;
		double exact;

		if (i < count) {
			x = sample(&state, test->low, test->high, (int)i);
		} else {
			x = swept;
			swept = nextafterf(swept, INFINITY);
			++sweep;
		}
		exact = test->reference(x);
		float got = test->function(x);
		double error = ulp_error(got, exact);

		if (got != (float)exact)
			++misrounded;
		if (error >= 1.0 && failures < 20) {
			snprintf(message, sizeof(message), "fail: %s(%a) = %a, exact %a\n", test->name, (double)x, (double)got, exact);
			fputs(message, stdout);
			++failures;
		}
		if (error > worst) {
			worst = error;
			worst_x = x;
		}
	}

	count += sweep;
	snprintf(message, sizeof(message), "%s: max %.3f ulp at %a, %ld of %ld not correctly rounded\n", test->name, worst, (double)worst_x, misrounded, count);
	fputs(message, stdout);
	/* Faithful everywhere, and correctly rounded but for rare near-halfway cases */
	if (worst >= 1.0 || misrounded > count / 1000)
		++failures;
}

int main(void)
{
	for (size_t i = 0; i < sizeof(unary) / sizeof(unary[0]); ++i)
		check_unary(&unary[i]);

	fputs(failures == 0 ? "float_accuracy: pass\n" : "float_accuracy: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
