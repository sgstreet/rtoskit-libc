/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * softdp.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "softdp.h"

/*
 * The double-precision operations and conversions (PLAN D99), on every
 * board through the compiler's own code: the FPU's instructions, libgcc's
 * helpers, or the soft-dp layer's on armv7e-m. Every implementation rounds
 * correctly to nearest, so all must give the same results bit for bit; they
 * are folded into one hash, whose expected value x86_64's hardware fixes.
 * NaNs count as one value, since their payloads may differ, and conversions
 * are checked only in range.
 */

#define EXPECTED_HASH 0x4bb357ad00334c52ULL

static int failures;

void fail(const char *what, uint64_t a, uint64_t b)
{
	char message[128];

	snprintf(message, sizeof(message), "fail: %s 0x%016llx 0x%016llx\n", what, (unsigned long long)a, (unsigned long long)b);
	fputs(message, stdout);
	++failures;
}

/* xorshift64: deterministic test values */
uint64_t next(uint64_t *state)
{
	*state ^= *state << 13;
	*state ^= *state >> 7;
	*state ^= *state << 17;
	return *state;
}

/*
 * Operand bits of several kinds: any, zeros and subnormals, near 1, any
 * exponent, a few significand bits above a run of ones or zeros (the
 * rounding's ties and carries), and sparse significands at every exponent.
 */
uint64_t operand(uint64_t *state, int kind)
{
	uint64_t r = next(state);
	uint64_t q = next(state);

	switch (kind % 6) {
		case 0:
			return r;
		case 1:
			return r & 0x800fffffffffffffULL;
		case 2:
			return (r & 0x800fffffffffffffULL) | (uint64_t)(1023 - 40 + q % 80) << 52;
		case 3:
			return (r & 0x800fffffffffffffULL) | (uint64_t)(q % 2047) << 52;
		case 4:
			return (r & 0x80000000000003ffULL) | (uint64_t)(1000 + q % 50) << 52 | ((q >> 20 & 1) != 0 ? 0x000ffffffffffc00ULL : 0);
		default:
			return (r & 0x8000000000000000ULL) | (uint64_t)(q % 2048) << 52 | (r & 0xf) << (q >> 16) % 49;
	}
}

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

static float from_bits_float(uint32_t bits)
{
	float x;

	memcpy(&x, &bits, sizeof(x));
	return x;
}

/* FNV-1a over 64-bit words */
static void mix(uint64_t *hash, uint64_t value)
{
	*hash = (*hash ^ value) * 0x100000001b3ULL;
}

static uint64_t canonical(double x)
{
	return isnan(x) ? 0x7ff8000000000000ULL : bits_of(x);
}

static uint64_t canonical_float(float x)
{
	return isnan(x) ? 0x7fc00000u : bits_of_float(x);
}

/* Through volatile operands, so each operation happens at run time */
static uint64_t hash_operations(int count)
{
	uint64_t state = 0x9e3779b97f4a7c15ULL;
	uint64_t hash = 0xcbf29ce484222325ULL;

	for (int i = 0; i < count; ++i) {
		uint64_t ua = operand(&state, i);
		uint64_t ub = i % 7 == 0 ? ua ^ (next(&state) & 0x80000000000000ffULL) : operand(&state, i / 6 + i);
		volatile double a = from_bits(ua);
		volatile double b = from_bits(ub);
		volatile float f = from_bits_float((uint32_t)ub);
		volatile uint64_t v = next(&state) >> (i % 64);
		double x = a;

		mix(&hash, canonical(a + b));
		mix(&hash, canonical(a - b));
		mix(&hash, canonical(a * b));
		mix(&hash, canonical(a / b));
		mix(&hash, (uint64_t)(a == b) | (uint64_t)(a < b) << 1 | (uint64_t)(a <= b) << 2 | (uint64_t)(a > b) << 3 | (uint64_t)(a >= b) << 4 | (uint64_t)isunordered(a, b) << 5);
		mix(&hash, canonical_float((float)a));
		mix(&hash, canonical((double)f));
		mix(&hash, bits_of((double)(int64_t)v) ^ bits_of((double)v) << 1 ^ bits_of((double)(int32_t)v) << 2 ^ bits_of((double)(uint32_t)v) << 3);
		if (!isnan(x) && fabs(x) < 0x1p31)
			mix(&hash, (uint64_t)(int32_t)a);
		if (!isnan(x) && x > -1.0 && x < 0x1p32)
			mix(&hash, (uint32_t)a);
		if (!isnan(x) && fabs(x) < 0x1p63)
			mix(&hash, (uint64_t)(int64_t)a);
		if (!isnan(x) && x > -1.0 && x < 0x1p64)
			mix(&hash, (uint64_t)a);
		if (!isnan(f) && fabsf(f) < 0x1p63f)
			mix(&hash, (uint64_t)(int64_t)f);
		if (!isnan(f) && f > -1.0f && f < 0x1p64f)
			mix(&hash, (uint64_t)f);
	}

	return hash;
}

/* Known answers for the cases that random operands rarely reach */
static double add(double x, double y)
{
	volatile double a = x;
	volatile double b = y;

	return a + b;
}

static double multiply(double x, double y)
{
	volatile double a = x;
	volatile double b = y;

	return a * b;
}

static double divide(double x, double y)
{
	volatile double a = x;
	volatile double b = y;

	return a / b;
}

static void test_known_answers(void)
{
	/* IEEE 754-2019 6.3: an exact zero sum is +0 in round to nearest; -0 + -0 is -0 */
	if (bits_of(add(1.0, -1.0)) != 0 || bits_of(add(-0.0, -0.0)) != bits_of(-0.0) || bits_of(add(-0.0, 0.0)) != 0)
		fail("add zeros", 0, 0);
	/* Ties to even, and the carry into the next binade */
	if (add(1.0, 0x1p-53) != 1.0 || add(1.0 + 0x1p-52, 0x1p-53) != 1.0 + 0x1p-51 || add(0x1.fffffffffffffp0, 0x1p-52) != 2.0)
		fail("add rounding", 0, 0);
	/* Overflow to infinity, the largest finite value kept, invalid operations */
	if (multiply(0x1p1023, 2.0) != INFINITY || multiply(0x1.fffffffffffffp1023, 1.0) != 0x1.fffffffffffffp1023 || !isnan(multiply(INFINITY, 0.0)))
		fail("multiply range", 0, 0);
	/* Subnormal results, rounded once on their own grid, and the rounding up into the normal range */
	if (multiply(0x1p-1000, 0x1p-74) != 0x1p-1074 || multiply(0x1p-1000, 0x1p-75) != 0.0 || multiply(0x1.8p-1000, 0x1p-75) != 0x1p-1074 || multiply(0x1.fffffffffffffp-1, 0x1p-1022) != 0x1p-1022)
		fail("multiply subnormal", 0, 0);
	if (divide(1.0, 3.0) != 0x1.5555555555555p-2 || divide(2.0, 3.0) != 0x1.5555555555555p-1 || divide(1.0, 0x1.fffffffffffffp0) != 0x1.0000000000001p-1)
		fail("divide rounding", 0, 0);
	if (divide(1.0, 0.0) != INFINITY || divide(-1.0, 0.0) != -INFINITY || !isnan(divide(0.0, 0.0)) || !isnan(divide(INFINITY, INFINITY)) || divide(1.0, INFINITY) != 0.0)
		fail("divide special", 0, 0);
	if (divide(0x1p-1074, 2.0) != 0.0 || divide(0x3p-1074, 2.0) != 0x1p-1073 || divide(0x1p1023, 0x1p-1) != INFINITY)
		fail("divide range", 0, 0);
}

int main(void)
{
	uint64_t hash = hash_operations(30000);

	if (hash != EXPECTED_HASH) {
		char message[64];

		snprintf(message, sizeof(message), "hash 0x%016llx\n", (unsigned long long)hash);
		fputs(message, stdout);
		fail("hash", hash, EXPECTED_HASH);
	}
	test_known_answers();
	test_layer();

	fputs(failures == 0 ? "softdp: pass\n" : "softdp: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
