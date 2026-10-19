/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_rem_pio2.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* The bits of 2/pi after the binary point, 32 per word, most significant first */
static const uint32_t two_over_pi[] = { 0xa2f9836e, 0x4e441529, 0xfc2757d1, 0xf534ddc0, 0xdb629599, 0x3c439041, 0xfe5163ab, 0xdebbc561, 0xb7246e3a, 0x424dd2e0, 0x06492eea, 0x09d1921c, 0xfe1deb1c, 0xb129a73e, 0xe88235f5, 0x2ebb4484, 0xe99c7026, 0xb45f7e41, 0x3991d639, 0x835339f4, 0x9c845f8b, 0xbdf9283b, 0x1ff897ff, 0xde05980f, 0xef2f118b, 0x5a0a6d1f, 0x6d367ecf, 0x27cb09b7, 0x4f463f66, 0x9e5fea2d, 0x7527bac7, 0xebe5f17b, 0x3d0739f7, 0x8a5292ea, 0x6bfb5fb1, 0x1f8d5d08, 0x56033046, 0xfc7b6bab, 0xf0cfbc20, 0x9af4361d };

/* count (at most 53) bits of the little-endian 32-bit limbs from bit position (negative positions are zeros) */
static uint64_t bits_at(const uint32_t *limbs, int limb_count, int position, int count)
{
	uint64_t value = 0;

	for (int i = count - 1; i >= 0; --i) {
		int bit = position + i;

		value <<= 1;
		if (bit >= 0 && bit / 32 < limb_count)
			value |= (limbs[bit / 32] >> (bit % 32)) & 1;
	}

	return value;
}

/*
 * Payne-Hanek: x = m 2^e (m a 53-bit integer) times seven words of 2/pi,
 * starting where the earlier bits only add multiples of 4 to x 2/pi; the
 * 277-bit product has at least 191 fraction bits, enough for the closest
 * double to a multiple of pi/2 (about 2^-61 relative away).
 */
static int payne_hanek(double x, struct __libc_dd *r)
{
	uint64_t bits = __libc_double_bits(x);
	int e = __libc_double_exponent(bits) - __LIBC_DOUBLE_BIAS - 52;
	uint64_t m = (bits & __LIBC_DOUBLE_FRACTION) | (1ULL << 52);
	int first = e - 1 > 1 ? e - 1 : 1;
	int word = (first - 1) / 32;
	int shift = 32 * (word + 7) - e;
	uint32_t product[9] = { 0 };
	uint32_t m_limbs[2] = { (uint32_t)m, (uint32_t)(m >> 32) };
	int quadrant;
	bool negative;
	int top;
	struct __libc_dd f;

	for (int i = 0; i < 7; ++i) {
		uint64_t carry = 0;
		uint32_t b = two_over_pi[word + 6 - i];

		for (int j = 0; j < 2; ++j) {
			uint64_t sum = (uint64_t)b * m_limbs[j] + product[i + j] + carry;

			product[i + j] = (uint32_t)sum;
			carry = sum >> 32;
		}
		for (int k = i + 2; carry != 0 && k < 9; ++k) {
			uint64_t sum = (uint64_t)product[k] + carry;

			product[k] = (uint32_t)sum;
			carry = sum >> 32;
		}
	}

	quadrant = (int)bits_at(product, 9, shift, 2);
	negative = bits_at(product, 9, shift - 1, 1) != 0;
	if (negative) {
		/* The fraction is at least 1/2: take 1 - fraction, and one more quadrant */
		uint64_t borrow = 0;

		++quadrant;
		for (int i = 0; i < 9; ++i) {
			uint64_t difference = (uint64_t)0 - product[i] - borrow;

			product[i] = (uint32_t)difference;
			borrow = (difference >> 32) != 0;
		}
	}

	top = shift - 1;
	while (top >= 0 && bits_at(product, 9, top, 1) == 0)
		--top;
	if (top < 0) {
		*r = (struct __libc_dd){ .hi = 0.0, .lo = 0.0 };
		return quadrant & 3;
	}

	f = __libc_dd_fast_sum(ldexp((double)bits_at(product, 9, top - 52, 53), top - 52 - shift), ldexp((double)bits_at(product, 9, top - 105, 53), top - 105 - shift));
	f = __libc_dd_multiply(f, __LIBC_PI_2);
	*r = negative ? __libc_dd_negate(f) : f;

	return quadrant & 3;
}

/*
 * Below 2^20, Cody-Waite with pi/2 in four parts of 33, 33, 53 and 53 bits:
 * n times either 33-bit part is exact, x - n p1 is exact (Sterbenz), and the
 * remainder keeps about 150 bits. Larger arguments use Payne-Hanek. Rounding
 * to nearest is expected (D47).
 */
int __libc_rem_pio2(double x, struct __libc_dd *r)
{
	const double p1 = 0x1.921fb54400000p+0;
	const double p2 = 0x1.0b4611a600000p-34;
	const double p3 = 0x1.3198a2e037073p-69;
	const double p4 = 0x1.129024e088a68p-123;
	double a = fabs(x);
	double n;
	int quadrant;
	struct __libc_dd t;
	struct __libc_dd p;
	struct __libc_dd u;

	if (a <= 0x1.921fb54442d18p-1) {
		*r = (struct __libc_dd){ .hi = x, .lo = 0.0 };
		return 0;
	}
	if (a >= 0x1p20) {
		quadrant = payne_hanek(a, r);
		if (x < 0) {
			*r = __libc_dd_negate(*r);
			quadrant = -quadrant;
		}
		return quadrant & 3;
	}

	n = floor(a * 0x1.45f306dc9c883p-1 + 0.5);
	t = __libc_dd_sum(a - n * p1, -(n * p2));
	p = __libc_dd_product(n, p3);
	u = __libc_dd_sum(t.hi, -p.hi);
	*r = __libc_dd_fast_sum(u.hi, u.lo + t.lo - p.lo - n * p4);
	quadrant = (int)n;
	if (x < 0) {
		*r = __libc_dd_negate(*r);
		quadrant = -quadrant;
	}

	return quadrant & 3;
}
