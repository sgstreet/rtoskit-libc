/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * assembly.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "softdp.h"

/*
 * The soft-dp layer's Thumb-2 assembly (PLAN D99, D101): __aeabi_dadd,
 * __aeabi_dsub, __aeabi_dmul and the comparisons against the C models
 * that x86_64 checks against its hardware, bit for bit; a NaN must be a quiet NaN, whose
 * payload may differ. The operands are pairs of every exponent difference
 * up to 80 and below 4, significands that cancel, carry or tie, zeros,
 * subnormals, infinities and NaNs, and products around the range's ends.
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

static bool is_nan(uint64_t bits)
{
	return (bits << 1) > 0xffe0000000000000ULL;
}

static void check(const char *what, uint64_t a, uint64_t b, uint64_t got, uint64_t want)
{
	if (got == want)
		return;
	if (is_nan(want) && is_nan(got) && (got & 0x0008000000000000ULL) != 0)
		return;
	fail(what, a, b);
}

/* Every predicate against the model's ordering */
static void check_compare(uint64_t ua, uint64_t ub)
{
	int order = reference_compare(ua, ub);
	double a = from_bits(ua);
	double b = from_bits(ub);

	if (__aeabi_dcmplt(a, b) != (order == REFERENCE_LESS) || __aeabi_dcmple(a, b) != (order == REFERENCE_LESS || order == REFERENCE_EQUAL) || __aeabi_dcmpeq(a, b) != (order == REFERENCE_EQUAL))
		fail("dcmp lt/le/eq", ua, ub);
	if (__aeabi_dcmpgt(a, b) != (order == REFERENCE_GREATER) || __aeabi_dcmpge(a, b) != (order == REFERENCE_GREATER || order == REFERENCE_EQUAL) || __aeabi_dcmpun(a, b) != (order == REFERENCE_UNORDERED))
		fail("dcmp gt/ge/un", ua, ub);
}

void test_layer(void)
{
	uint64_t state = 0x9e3779b97f4a7c15ULL;

	for (int i = 0; i < 400000; ++i) {
		uint64_t r = next(&state);
		uint64_t q = next(&state);
		uint64_t t = next(&state);
		int ea = (int)(q % (i % 3 == 0 ? 4 : 2048));
		int d = (int)(t % ((i & 1) != 0 ? 4 : 80));
		int eb = ea - d < 0 ? (int)(t >> 20) % 2048 : ea - d;
		int ec = 1023 - ea + (int)(t % 140) - 70;
		uint64_t ma = r & 0xfffffffffffffULL;
		uint64_t mb = (r >> 7 ^ t) & 0xfffffffffffffULL;
		uint64_t ua;
		uint64_t ub;
		uint64_t uc;

		switch (i % 5) {
			case 1:
				/* Close significands: cancellation */
				mb = ma ^ (t >> 40);
				break;
			case 2:
				/* A run of ones above a few bits: carries and ties */
				ma |= 0xffffffffff000ULL;
				mb &= 0xfULL << (t % 48);
				break;
			case 3:
				ma &= 0xfULL << (q % 48);
				mb |= 0xfffffffff0000ULL;
				break;
			case 4:
				mb = (q & 1) != 0 ? 0 : 0xfffffffffffffULL;
				break;
			default:
				break;
		}
		ua = (r & 0x8000000000000000ULL) | (uint64_t)ea << 52 | ma;
		ub = (q & 0x8000000000000000ULL) | (uint64_t)eb << 52 | mb;
		if (i % 19 == 0)
			ub = ua ^ (t & 0x8000000000000001ULL);
		if (i % 23 == 0)
			ua &= 0x800fffffffffffffULL;
		if (i % 31 == 0)
			ub &= 0x800fffffffffffffULL >> (t % 50);
		if (i % 37 == 0)
			ua |= 0x7ff0000000000000ULL;
		if (i % 41 == 0)
			ub = (ub & 0x8000000000000000ULL) | 0x7ff0000000000000ULL;
		if (i % 43 == 0)
			ua = (ua & 0x8000000000000000ULL) | 0x7ff0000000000000ULL;
		if (i % 13 == 0)
			ub &= 0x8000000000000000ULL;
		if (i % 17 == 0)
			ua &= 0x8000000000000000ULL;
		if ((i & 2) != 0) {
			uint64_t swap = ua;

			ua = ub;
			ub = swap;
		}
		ec = ec < 0 ? 0 : ec > 2047 ? 2047 :
									  ec;
		uc = (t & 0x8000000000000000ULL) | (uint64_t)((i & 4) != 0 ? ec : (int)(r % 2048)) << 52 | mb;

		check("dadd", ua, ub, bits_of(__aeabi_dadd(from_bits(ua), from_bits(ub))), bits_of(reference_add(from_bits(ua), from_bits(ub))));
		check("dsub", ua, ub, bits_of(__aeabi_dsub(from_bits(ua), from_bits(ub))), bits_of(reference_add(from_bits(ua), from_bits(ub ^ 0x8000000000000000ULL))));
		check("dmul", ua, uc, bits_of(__aeabi_dmul(from_bits(ua), from_bits(uc))), bits_of(reference_multiply(from_bits(ua), from_bits(uc))));
		check_compare(ua, ub);
		check_compare(ua, ua ^ (t & 1));
		check_compare(ua & 0x8000000000000000ULL, ub & 0x8000000000000000ULL);
	}
}
