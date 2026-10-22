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
#include "softdouble.h"
#include "softdp.h"

/*
 * The soft-dp layer's helpers (PLAN D99), compiled into this test on
 * x86_64 and called directly, against the hardware's operations case by
 * case: the C ones, and for the addition, the multiplication and the
 * narrowing conversion, whose fast paths are Thumb-2 assembly, the general
 * C versions those fall back on (the conversions written in assembly alone
 * are checked on the Arm cores, by the hash). The hash in softdp.c covers the layer as the Arm cores link it.
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
	if (__aeabi_dcmpeq(a, b) != (a == b) || __aeabi_dcmplt(a, b) != (a < b) || __aeabi_dcmple(a, b) != (a <= b) || __aeabi_dcmpgt(a, b) != (a > b) || __aeabi_dcmpge(a, b) != (a >= b) || __aeabi_dcmpun(a, b) != isunordered(a, b))
		fail("dcmp", ua, ub);
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

	/* The FPU's saturation out of range, which the helpers copy */
	if (__aeabi_d2lz(0x1p70) != INT64_MAX || __aeabi_d2lz(NAN) != 0 || __aeabi_d2ulz(-2.0) != 0 || __aeabi_d2ulz(0x1p70) != UINT64_MAX || __aeabi_f2lz(-0x1p70f) != INT64_MIN)
		fail("saturation", 0, 0);
}

void test_layer(void)
{
	static const uint64_t special[] = { 0, 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0xfff0000000000000ULL, 0x7ff8000000000000ULL, 0x7ff4000000000001ULL, 1, 0x800fffffffffffffULL, 0x0010000000000000ULL, 0x7fefffffffffffffULL, 0x3ff0000000000000ULL, 0xbff0000000000000ULL, 0x3fe0000000000000ULL, 0x4340000000000000ULL, 0x3ca0000000000000ULL, 0x000fffffffffffffULL, 0x41e0000000000000ULL, 0xc1e0000000000000ULL, 0x43e0000000000000ULL };
	int count = sizeof(special) / sizeof(special[0]);
	uint64_t state = 0x2545f4914f6cdd1dULL;

	for (int i = 0; i < count; ++i)
		for (int k = 0; k < count; ++k)
			check(from_bits(special[i]), from_bits(special[k]), (float)from_bits(special[k]), special[i]);

	for (int i = 0; i < 300000; ++i) {
		uint64_t ua = operand(&state, i);
		uint64_t ub = i % 7 == 0 ? ua ^ (next(&state) & 0x80000000000000ffULL) : operand(&state, i / 6 + i);
		uint32_t fb = (uint32_t)ub;
		float f;

		memcpy(&f, &fb, sizeof(f));
		check(from_bits(ua), from_bits(ub), f, next(&state) >> (i % 64));
	}
}
