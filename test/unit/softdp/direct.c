/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * direct.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "internal/aeabi.h"
#include "internal/softdouble.h"
#include "softdp.h"

/*
 * The soft-dp, soft-int and nofp layers' C helpers (PLAN D99, D102),
 * compiled into this test on x86_64 and called directly, against the
 * hardware's operations case by case. For the addition, the
 * multiplication, the comparisons and the narrowing conversion, whose fast
 * paths are Thumb-2 assembly, the C models and the general C versions the
 * assembly falls back on stand in. The conversions written in assembly
 * alone are checked on the Arm cores, by the hash in softdp.c, which covers
 * the layers as those cores link them.
 */
static uint64_t bits_of(double x)
{
	uint64_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static double from_bits(uint64_t bits)
{
	double x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

static uint32_t bits_of_float(float x)
{
	uint32_t bits;

	memcpy(&bits, &x, sizeof(bits));
	return bits;
}

static bool same(double a, double b)
{
	return (isnan(a) && isnan(b)) || bits_of(a) == bits_of(b);
}

static bool same_float(float a, float b)
{
	return (isnan(a) && isnan(b)) || bits_of_float(a) == bits_of_float(b);
}

static void check(double x, double y, float f, uint64_t v)
{
	volatile double a = x;
	volatile double b = y;
	uint64_t ua = bits_of(x);
	uint64_t ub = bits_of(y);

	if (!same(reference_add(a, b), a + b) || !same(reference_add(a, -b), a - b))
		fail("dadd/dsub", ua, ub);
	if (!same(reference_multiply(a, b), a * b))
		fail("dmul", ua, ub);
	if (!same(__aeabi_ddiv(a, b), a / b))
		fail("ddiv", ua, ub);
	{
		int order = reference_compare(ua, ub);

		if ((order == REFERENCE_LESS) != (a < b) || (order == REFERENCE_EQUAL) != (a == b) || (order == REFERENCE_GREATER) != (a > b) || (order == REFERENCE_UNORDERED) != isunordered(a, b))
			fail("compare model", ua, ub);
	}
	if (__aeabi_dcmpun(a, b) != isunordered(a, b))
		fail("dcmpun", ua, ub);
	if (!same_float(__libc_softdouble_d2f(a), (float)a))
		fail("d2f", ua, 0);
	if (!same(__aeabi_i2d((int)v), (double)(int)v) || !same(__aeabi_ui2d((unsigned int)v), (double)(unsigned int)v))
		fail("int to double", v, 0);
	if (!isnan(x) && fabs(x) < 0x1p63 && __aeabi_d2lz(a) != (long long)a)
		fail("d2lz", ua, 0);
	if (!isnan(x) && x > -1.0 && x < 0x1p64 && __aeabi_d2ulz(a) != (unsigned long long)a)
		fail("d2ulz", ua, 0);
	if (!isnan(f) && fabsf(f) < 0x1p63f && __aeabi_f2lz(f) != (long long)f)
		fail("f2lz", bits_of_float(f), 0);
	if (!isnan(f) && f > -1.0f && f < 0x1p64f && __aeabi_f2ulz(f) != (unsigned long long)f)
		fail("f2ulz", bits_of_float(f), 0);
	if (!isnan(x) && x > -1.0 && x < 0x1p32 && __aeabi_d2uiz(a) != (unsigned int)a)
		fail("d2uiz", ua, 0);
	if (!isnan(f) && f > -1.0f && f < 0x1p32f && __aeabi_f2uiz(f) != (unsigned int)f)
		fail("f2uiz", bits_of_float(f), 0);

	/* The models of the conversions written only in assembly */
	if (reference_f2d(bits_of_float(f)) != bits_of((double)f) && !(isnan(f) && (reference_f2d(bits_of_float(f)) & 0x0008000000000000ULL) != 0))
		fail("f2d model", bits_of_float(f), 0);
	if (!isnan(x) && fabs(x) < 0x1p31 && reference_d2iz(ua) != (int)a)
		fail("d2iz model", ua, 0);
	if (!isnan(x) && x > -1.0 && x < 0x1p32 && reference_d2uiz(ua) != (unsigned int)a)
		fail("d2uiz model", ua, 0);
	if (reference_l2d((int64_t)v) != bits_of((double)(long long)v) || reference_ul2d(0, v) != bits_of((double)v))
		fail("l2d/ul2d model", v, 0);

	/* The FPU's saturation out of range, which the helpers copy */
	if (__aeabi_d2lz(0x1p70) != INT64_MAX || __aeabi_d2lz(NAN) != 0 || __aeabi_d2ulz(-2.0) != 0 || __aeabi_d2ulz(0x1p70) != UINT64_MAX || __aeabi_f2lz(-0x1p70f) != INT64_MIN || __aeabi_d2uiz(0x1p40) != UINT32_MAX || __aeabi_d2uiz(-0.5) != 0 || __aeabi_f2uiz(NAN) != 0 || __aeabi_f2uiz(0x1p40f) != UINT32_MAX)
		fail("saturation", 0, 0);
}

void test_layer(unsigned long iterations, unsigned long float_stride)
{
	static const uint64_t special[] = { 0, 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0xfff0000000000000ULL, 0x7ff8000000000000ULL, 0x7ff4000000000001ULL, 1, 0x800fffffffffffffULL, 0x0010000000000000ULL, 0x7fefffffffffffffULL, 0x3ff0000000000000ULL, 0xbff0000000000000ULL, 0x3fe0000000000000ULL, 0x4340000000000000ULL, 0x3ca0000000000000ULL, 0x000fffffffffffffULL, 0x41e0000000000000ULL, 0xc1e0000000000000ULL, 0x43e0000000000000ULL };
	int count = sizeof(special) / sizeof(special[0]);
	uint64_t state = 0x2545f4914f6cdd1dULL;

	for (int i = 0; i < count; ++i)
		for (int k = 0; k < count; ++k)
			check(from_bits(special[i]), from_bits(special[k]), (float)from_bits(special[k]), special[i]);

	for (unsigned long i = 0; i < iterations; ++i) {
		uint64_t ua = operand(&state, i);
		uint64_t ub = i % 7 == 0 ? ua ^ (next(&state) & 0x80000000000000ffULL) : operand(&state, i / 6 + i);
		uint32_t fb = (uint32_t)ub;
		float f;

		memcpy(&f, &fb, sizeof(f));
		check(from_bits(ua), from_bits(ub), f, next(&state) >> (i % 64));
	}

	/* Every float_stride-th float, and the two after it, through the model of the conversion from float */
	for (uint64_t g = 0; g < (1ULL << 32); g += float_stride) {
		for (uint32_t k = 0; k < 3; ++k) {
			uint32_t bits = (uint32_t)g + k;
			float h;

			memcpy(&h, &bits, sizeof(h));
			if (reference_f2d(bits) != bits_of((double)h) && !(isnan(h) && (reference_f2d(bits) & 0x0008000000000000ULL) != 0))
				fail("f2d model sweep", bits, 0);
		}
	}
}
