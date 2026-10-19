/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fpu_math.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "generic.h"

/*
 * The FPU-layer functions (D46: VRINT*, VMAXNM/VMINNM, VFMA, VABS, VSQRT on
 * fpv5-sp/fpv5-dp, VFMA, VABS and VSQRT on fpv4-sp, SQRTSD/SQRTSS on x86_64)
 * against the generic C versions: the same result bits (any NaN for a NaN)
 * and the same exception flags, in every rounding direction the core
 * supports. Floats are sampled
 * through the whole encoding space with a prime stride, doubles at random,
 * both with the special and halfway values added.
 */

static int failures;

static void fail(const char *name, uint64_t x, int direction, uint64_t got, uint64_t want, int got_flags, int want_flags)
{
	if (++failures <= 20)
		printf("fail: %s(%016llx) direction %d = %016llx flags %02x, generic %016llx flags %02x\n", name, (unsigned long long)x, direction, (unsigned long long)got, got_flags, (unsigned long long)want, want_flags);
}

static uint64_t state = 0x9e3779b97f4a7c15ULL;

static uint64_t next_random(void)
{
	state ^= state << 13;
	state ^= state >> 7;
	state ^= state << 17;
	return state;
}

static uint32_t bits_f(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static uint64_t bits_d(double x)
{
	uint64_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static float from_f(uint32_t bits)
{
	float x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

static double from_d(uint64_t bits)
{
	double x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

typedef float (*unary_f)(float);
typedef double (*unary_d)(double);

static const struct {
	const char *name;
	unary_f library;
	unary_f generic;
} unary_float[] = {
	{ "fabsf", fabsf, generic_fabsf },
	{ "rintf", rintf, generic_rintf },
	{ "nearbyintf", nearbyintf, generic_nearbyintf },
	{ "truncf", truncf, generic_truncf },
	{ "floorf", floorf, generic_floorf },
	{ "ceilf", ceilf, generic_ceilf },
	{ "roundf", roundf, generic_roundf },
	{ "roundevenf", roundevenf, generic_roundevenf },
	{ "sqrtf", sqrtf, generic_sqrtf },
};

static const struct {
	const char *name;
	unary_d library;
	unary_d generic;
} unary_double[] = {
	{ "fabs", fabs, generic_fabs },
	{ "rint", rint, generic_rint },
	{ "nearbyint", nearbyint, generic_nearbyint },
	{ "trunc", trunc, generic_trunc },
	{ "floor", floor, generic_floor },
	{ "ceil", ceil, generic_ceil },
	{ "round", round, generic_round },
	{ "roundeven", roundeven, generic_roundeven },
	{ "sqrt", sqrt, generic_sqrt },
};

/* Run f in a direction; the flags it raised */
#define MEASURE(result, flags, call)         \
	do {                                     \
		feclearexcept(FE_ALL_EXCEPT);        \
		result = call;                       \
		flags = fetestexcept(FE_ALL_EXCEPT); \
	} while (false)

static bool same_f(float a, float b)
{
	return (isnan(a) && isnan(b)) || bits_f(a) == bits_f(b);
}

static bool same_d(double a, double b)
{
	return (isnan(a) && isnan(b)) || bits_d(a) == bits_d(b);
}

static const uint32_t special_f[] = { 0x00000000, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000, 0x7fa00001, 0xffc00001, 0x00000001, 0x807fffff, 0x00800000, 0x3f000000, 0xbf000000, 0x3fc00000, 0x40200000, 0xc0200000, 0x4b000000, 0x4affffff, 0x4b7fffff, 0x3effffff, 0x7f7fffff };
static const uint64_t special_d[] = { 0, 0x8000000000000000, 0x7ff0000000000000, 0xfff0000000000000, 0x7ff8000000000000, 0x7ff4000000000001, 0x0000000000000001, 0x800fffffffffffff, 0x0010000000000000, 0x3fe0000000000000, 0xbff8000000000000, 0x4004000000000000, 0x4330000000000000, 0x432fffffffffffff, 0x433fffffffffffff, 0x3fdfffffffffffff, 0x7fefffffffffffff };

static void check_float(uint32_t x_bits, int direction)
{
	float x = from_f(x_bits);
	float y = from_f(special_f[x_bits % (sizeof(special_f) / sizeof(special_f[0]))]);
	float z = from_f((uint32_t)next_random());
	float got;
	float want;
	int got_flags;
	int want_flags;

	for (size_t i = 0; i < sizeof(unary_float) / sizeof(unary_float[0]); ++i) {
		MEASURE(got, got_flags, unary_float[i].library(x));
		MEASURE(want, want_flags, unary_float[i].generic(x));
		if (!same_f(got, want) || got_flags != want_flags)
			fail(unary_float[i].name, x_bits, direction, bits_f(got), bits_f(want), got_flags, want_flags);
	}
	MEASURE(got, got_flags, fmaxf(x, y));
	MEASURE(want, want_flags, generic_fmaxf(x, y));
	if (!same_f(got, want) || got_flags != want_flags)
		fail("fmaxf", x_bits, direction, bits_f(got), bits_f(want), got_flags, want_flags);
	MEASURE(got, got_flags, fminf(y, x));
	MEASURE(want, want_flags, generic_fminf(y, x));
	if (!same_f(got, want) || got_flags != want_flags)
		fail("fminf", x_bits, direction, bits_f(got), bits_f(want), got_flags, want_flags);
	MEASURE(got, got_flags, fmaf(x, z, y));
	MEASURE(want, want_flags, generic_fmaf(x, z, y));
	if (!same_f(got, want) || got_flags != want_flags)
		fail("fmaf", x_bits, direction, bits_f(got), bits_f(want), got_flags, want_flags);
}

static void check_double(uint64_t x_bits, int direction)
{
	double x = from_d(x_bits);
	double y = from_d(special_d[x_bits % (sizeof(special_d) / sizeof(special_d[0]))]);
	double z = from_d(next_random());
	double w = from_d(next_random());
	double got;
	double want;
	int got_flags;
	int want_flags;

	for (size_t i = 0; i < sizeof(unary_double) / sizeof(unary_double[0]); ++i) {
		MEASURE(got, got_flags, unary_double[i].library(x));
		MEASURE(want, want_flags, unary_double[i].generic(x));
		if (!same_d(got, want) || got_flags != want_flags)
			fail(unary_double[i].name, x_bits, direction, bits_d(got), bits_d(want), got_flags, want_flags);
	}
	MEASURE(got, got_flags, fmax(x, y));
	MEASURE(want, want_flags, generic_fmax(x, y));
	if (!same_d(got, want) || got_flags != want_flags)
		fail("fmax", x_bits, direction, bits_d(got), bits_d(want), got_flags, want_flags);
	MEASURE(got, got_flags, fmin(y, x));
	MEASURE(want, want_flags, generic_fmin(y, x));
	if (!same_d(got, want) || got_flags != want_flags)
		fail("fmin", x_bits, direction, bits_d(got), bits_d(want), got_flags, want_flags);
	/* Random fma operands rarely interact; scale z near x * w to reach cancellation and rounding cases */
	MEASURE(got, got_flags, fma(x, w, z));
	MEASURE(want, want_flags, generic_fma(x, w, z));
	if (!same_d(got, want) || got_flags != want_flags)
		fail("fma", x_bits, direction, bits_d(got), bits_d(want), got_flags, want_flags);
	z = -(x * w);
	MEASURE(got, got_flags, fma(x, w, z));
	MEASURE(want, want_flags, generic_fma(x, w, z));
	if (!same_d(got, want) || got_flags != want_flags)
		fail("fma (cancelling)", x_bits, direction, bits_d(got), bits_d(want), got_flags, want_flags);
}

int main(void)
{
	long long checked = 0;

	for (int direction = 0; direction < 4; ++direction) {
		/* FE_* values 0-3: nearest, upward, downward, toward zero; a core without directed rounding has only 0 */
		if (fesetround(direction) != 0)
			continue;
		for (size_t i = 0; i < sizeof(special_f) / sizeof(special_f[0]); ++i)
			check_float(special_f[i], direction);
		for (uint64_t bits = 0; bits <= UINT32_MAX; bits += 65521) {
			check_float((uint32_t)bits, direction);
			++checked;
		}
		for (size_t i = 0; i < sizeof(special_d) / sizeof(special_d[0]); ++i)
			check_double(special_d[i], direction);
		for (int i = 0; i < 40000; ++i) {
			uint64_t bits = next_random();

			/* Half the samples with exponents near 2^52, where rounding to integers is decided */
			if ((i & 1) != 0)
				bits = (bits & 0x800fffffffffffffULL) | ((uint64_t)(0x3fe + (bits >> 52) % 60) << 52);
			check_double(bits, direction);
			++checked;
		}
	}
	fesetround(FE_TONEAREST);

	if (failures == 0)
		printf("pass: fpu_math (%lld samples)\n", checked);

	return failures == 0 ? 0 : 1;
}
