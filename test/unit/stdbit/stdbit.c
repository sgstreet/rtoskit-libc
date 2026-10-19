/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdbit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <stdbit.h>
#include <stdint.h>
#include <stdio.h>

/*
 * <stdbit.h> (P1.03): every typed function and the type-generic macros
 * against bit-by-bit reference loops, for 0, 1, the maximum, every power of
 * two and its neighbours, and a pseudo-random sweep, in every type.
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

/* Reference results for the low "width" bits of value, computed one bit at a time */
struct reference {
	unsigned int leading_zeros;
	unsigned int leading_ones;
	unsigned int trailing_zeros;
	unsigned int trailing_ones;
	unsigned int first_leading_zero;
	unsigned int first_leading_one;
	unsigned int first_trailing_zero;
	unsigned int first_trailing_one;
	unsigned int count_zeros;
	unsigned int count_ones;
	bool has_single_bit;
	unsigned int bit_width;
	unsigned long long bit_floor;
	unsigned long long bit_ceil;
};

static struct reference compute(unsigned long long value, unsigned int width)
{
	struct reference r = {};
	unsigned int i;

	/* Bit i from the top is bit (width - 1 - i) from the bottom */
	for (i = 0; i < width; ++i)
		if ((value >> (width - 1 - i)) & 1)
			break;
	r.leading_zeros = i;
	for (i = 0; i < width; ++i)
		if (!((value >> (width - 1 - i)) & 1))
			break;
	r.leading_ones = i;
	for (i = 0; i < width; ++i)
		if ((value >> i) & 1)
			break;
	r.trailing_zeros = i;
	for (i = 0; i < width; ++i)
		if (!((value >> i) & 1))
			break;
	r.trailing_ones = i;

	/* "First" results are 1-based indexes, 0 when there is no such bit */
	r.first_leading_zero = r.leading_ones < width ? r.leading_ones + 1 : 0;
	r.first_leading_one = r.leading_zeros < width ? r.leading_zeros + 1 : 0;
	r.first_trailing_zero = r.trailing_ones < width ? r.trailing_ones + 1 : 0;
	r.first_trailing_one = r.trailing_zeros < width ? r.trailing_zeros + 1 : 0;

	for (i = 0; i < width; ++i)
		r.count_ones += (value >> i) & 1;
	r.count_zeros = width - r.count_ones;
	r.has_single_bit = r.count_ones == 1;
	r.bit_width = width - r.leading_zeros;
	r.bit_floor = value == 0 ? 0 : 1ULL << (r.bit_width - 1);
	if (value <= 1)
		r.bit_ceil = 1;
	else if (r.has_single_bit)
		r.bit_ceil = value;
	else
		r.bit_ceil = r.bit_width < width ? 1ULL << r.bit_width : 0;

	return r;
}

#define CHECK_TYPE(suffix, type, value, width)                                                                                        \
	do {                                                                                                                              \
		type v = (type)(value);                                                                                                       \
		struct reference r = compute(v, width);                                                                                       \
		CHECK(stdc_leading_zeros_##suffix(v) == r.leading_zeros && stdc_leading_zeros(v) == r.leading_zeros);                         \
		CHECK(stdc_leading_ones_##suffix(v) == r.leading_ones && stdc_leading_ones(v) == r.leading_ones);                             \
		CHECK(stdc_trailing_zeros_##suffix(v) == r.trailing_zeros && stdc_trailing_zeros(v) == r.trailing_zeros);                     \
		CHECK(stdc_trailing_ones_##suffix(v) == r.trailing_ones && stdc_trailing_ones(v) == r.trailing_ones);                         \
		CHECK(stdc_first_leading_zero_##suffix(v) == r.first_leading_zero && stdc_first_leading_zero(v) == r.first_leading_zero);     \
		CHECK(stdc_first_leading_one_##suffix(v) == r.first_leading_one && stdc_first_leading_one(v) == r.first_leading_one);         \
		CHECK(stdc_first_trailing_zero_##suffix(v) == r.first_trailing_zero && stdc_first_trailing_zero(v) == r.first_trailing_zero); \
		CHECK(stdc_first_trailing_one_##suffix(v) == r.first_trailing_one && stdc_first_trailing_one(v) == r.first_trailing_one);     \
		CHECK(stdc_count_zeros_##suffix(v) == r.count_zeros && stdc_count_zeros(v) == r.count_zeros);                                 \
		CHECK(stdc_count_ones_##suffix(v) == r.count_ones && stdc_count_ones(v) == r.count_ones);                                     \
		CHECK(stdc_has_single_bit_##suffix(v) == r.has_single_bit && stdc_has_single_bit(v) == r.has_single_bit);                     \
		CHECK(stdc_bit_width_##suffix(v) == r.bit_width && stdc_bit_width(v) == r.bit_width);                                         \
		CHECK(stdc_bit_floor_##suffix(v) == r.bit_floor && stdc_bit_floor(v) == r.bit_floor);                                         \
		CHECK(stdc_bit_ceil_##suffix(v) == r.bit_ceil && stdc_bit_ceil(v) == r.bit_ceil);                                             \
	} while (false)

static void check_value(unsigned long long value)
{
	CHECK_TYPE(uc, unsigned char, value, UCHAR_WIDTH);
	CHECK_TYPE(us, unsigned short, value, USHRT_WIDTH);
	CHECK_TYPE(ui, unsigned int, value, UINT_WIDTH);
	CHECK_TYPE(ul, unsigned long, value, ULONG_WIDTH);
	CHECK_TYPE(ull, unsigned long long, value, ULLONG_WIDTH);
}

int main(void)
{
	unsigned long long state = 0x9e3779b97f4a7c15ULL;

	check_value(0);
	check_value(ULLONG_MAX);
	for (unsigned int bit = 0; bit < 64; ++bit) {
		check_value(1ULL << bit);
		check_value((1ULL << bit) - 1);
		check_value((1ULL << bit) + 1);
		check_value(~(1ULL << bit));
	}
	for (int i = 0; i < 2000; ++i) {
		state = state * 6364136223846793005ULL + 1442695040888963407ULL;
		check_value(state);
		check_value(state >> (state & 63));
	}

	/* The generic forms keep the argument's type for floor and ceiling, and fold for constants */
	static_assert(_Generic(stdc_bit_ceil((unsigned char)3), unsigned char: 1, default: 0));
	static_assert(_Generic(stdc_bit_floor(3UL), unsigned long: 1, default: 0));
	static_assert(stdc_bit_width(255U) == 8);
	static_assert(__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__ || __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__);
	static_assert(__STDC_ENDIAN_LITTLE__ != __STDC_ENDIAN_BIG__);
	static_assert(__STDC_VERSION_STDBIT_H__ == 202311L);

	/* The native byte order matches memory */
	uint32_t word = 0x01020304;
	unsigned char first = *(unsigned char *)&word;

	CHECK(first == (__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__ ? 0x04 : 0x01));

	if (failures == 0)
		puts("pass: stdbit");

	return failures == 0 ? 0 : 1;
}
