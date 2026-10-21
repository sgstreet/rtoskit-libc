/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_digits.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/div64.h"
#include "internal/format.h"

/*
 * The speed flavour's digit writer (PLAN D77, docs/research/conversions-speed.md).
 * A decimal value above 32 bits is split into chunks of 10^9, so at most two
 * 64-bit divisions happen where the size flavour does one per digit; each
 * chunk is written two digits per 32-bit division by 100, the pair split
 * as (r * 205) >> 11 (exact below 1029) and the rest. The power-of-two
 * bases shift a 32-bit copy once the value fits one. The rest takes the
 * size flavour's code unchanged: for 32-bit values pairs saved
 * nothing on cores with a multiply-high (the compiler's x / 10 is already
 * one UMULL) and cost a multiplication helper on Armv6-M, so the pair code
 * sits in a function of its own that costs the common path nothing.
 */

/* Writes the decimal digits of x backwards ending at p */
static char *decimal(char *p, uint32_t x)
{
	while (x >= 100) {
		uint32_t q = x / 100;
		uint32_t r = x - q * 100;
		uint32_t tens = (r * 205) >> 11;

		p -= 2;
		p[0] = (char)('0' + tens);
		p[1] = (char)('0' + r - tens * 10);
		x = q;
	}
	if (x >= 10) {
		uint32_t tens = (x * 205) >> 11;

		p -= 2;
		p[0] = (char)('0' + tens);
		p[1] = (char)('0' + x - tens * 10);
	} else if (x != 0) {
		*--p = (char)('0' + x);
	}

	return p;
}

/* Decimal values above 32 bits: chunks of nine digits below the top, each written in full, then the top */
[[gnu::noinline]] static char *wide_decimal(char *p, uintmax_t value)
{
	while (value > UINT32_MAX) {
		char *start = p - 9;
		uint32_t chunk;

		value = __libc_udivmod64(value, 1000000000U, &chunk);
		p = decimal(p, chunk);
		while (p > start)
			*--p = '0';
	}

	return decimal(p, (uint32_t)value);
}

/* Writes the digits of value backwards ending at end; returns the first digit */
char *__libc_format_digits(char *end, uintmax_t value, char conversion)
{
	const char *table = conversion == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
	char *p = end;
	unsigned int shift;
	uint32_t small;

	switch (conversion) {
		case 'b':
		case 'B':
			shift = 1;
			break;
		case 'o':
			shift = 3;
			break;
		case 'x':
		case 'X':
		case 'p':
			shift = 4;
			break;
		default:
			if (value > UINT32_MAX)
				return wide_decimal(p, value);
			for (small = value; small != 0; small /= 10)
				*--p = '0' + small % 10;
			return p;
	}

	/* The power-of-two bases shift a 32-bit copy while the value fits one (a 64-bit shift is a helper call on Armv6-M) */
	for (; value > UINT32_MAX; value >>= shift)
		*--p = table[value & ((1U << shift) - 1)];
	for (small = value; small != 0; small >>= shift)
		*--p = table[small & ((1U << shift) - 1)];

	return p;
}
