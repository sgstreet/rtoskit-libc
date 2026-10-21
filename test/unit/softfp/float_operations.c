/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * float_operations.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/aeabi.h"
#include "internal/math.h"
#include "softfp.h"

/*
 * The single-precision multiplication and division helpers (PLAN D85): the
 * integer ones of the nofp layer on Armv6-M (and compiled into this test on
 * x86_64), the compiler's elsewhere; and sqrtf, in integers on Armv6-M.
 * Rounding the double result to float is correct rounding, since double
 * has more than 2 x 24 + 2 bits.
 */
static void check_float_operation(float a, float b)
{
	/* Through volatile doubles: GCC would narrow (float)((double)a * (double)b) to the float operation under test */
	volatile double wide_a = a;
	volatile double wide_b = b;
	float product = __aeabi_fmul(a, b);
	float quotient = __aeabi_fdiv(a, b);
	float want_product = (float)(wide_a * wide_b);
	float want_quotient = (float)(wide_a / wide_b);

	if (!(isnan(product) && isnan(want_product)) && bits_of_float(product) != bits_of_float(want_product))
		fail("fmul", bits_of_float(a) | (uint64_t)bits_of_float(b) << 32);
	if (!(isnan(quotient) && isnan(want_quotient)) && bits_of_float(quotient) != bits_of_float(want_quotient))
		fail("fdiv", bits_of_float(a) | (uint64_t)bits_of_float(b) << 32);
}

static void check_square_root(float a)
{
	float root = sqrtf(a);
	float want = (float)sqrt((double)a);

	if (!(isnan(root) && isnan(want)) && bits_of_float(root) != bits_of_float(want))
		fail("sqrtf", bits_of_float(a));
}

void test_float_operations(void)
{
	static const uint32_t special[] = { 0x00000000, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000, 0x00000001, 0x807fffff, 0x00800000, 0x7f7fffff, 0x3f800000, 0xbf800000, 0x3f000000, 0x4b800000, 0x33800000 };
	uint64_t state = 0x2545f4914f6cdd1dULL;
	int count = sizeof(special) / sizeof(special[0]);

	for (int i = 0; i < count; ++i) {
		check_square_root(__libc_float_from(special[i]));
		for (int k = 0; k < count; ++k)
			check_float_operation(__libc_float_from(special[i]), __libc_float_from(special[k]));
	}

	/* Every float in [1, 4) with a significand ending in 0x1ff: both exponent parities, every leading digit */
	for (uint32_t bits = 0x3f8001ff; bits < 0x40800000; bits += 0x200)
		check_square_root(__libc_float_from(bits));

	/* Random operands: any bits, subnormals, and pairs whose results reach the ends of the range */
	for (int i = 0; i < 40000; ++i) {
		uint64_t r = next(&state);
		uint32_t a = (uint32_t)r;
		uint32_t b = (uint32_t)(r >> 32);

		if (i % 4 == 1) {
			a &= 0x807fffff;
		} else if (i % 4 == 2) {
			b = (b & 0x80ffffff) | (uint32_t)(64 + (r >> 40) % 128) << 23;
		} else if (i % 4 == 3) {
			/*
			 * Near 1, times 1 + m 2^-23 or 2 - (m + 1) 2^-23: the product's bits below the
			 * significand are a's own few bits, so the guard and a lone sticky bit at every
			 * position reach the rounding, in both normalisations
			 */
			uint32_t m = (uint32_t)(r >> 20) % 4;

			a = (a & 0x80000000) | (uint32_t)(112 + (r >> 26) % 32) << 23 | (uint32_t)(r >> 16 & 3) << 21 | 1u << (r >> 8) % 23 | (uint32_t)(r >> 18 & 1);
			b = (b & 0x80000000) | (uint32_t)(112 + (r >> 32) % 32) << 23 | ((r >> 22 & 1) != 0 ? 0x7fffff - m : 1 + m);
		}
		check_float_operation(__libc_float_from(a), __libc_float_from(b));
		check_square_root(__libc_float_from(a));
	}
}
