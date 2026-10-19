/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtofloat.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_STRTOFLOAT_H
#define _INTERNAL_STRTOFLOAT_H

#include <stdint.h>

#include "internal/decimal.h"

/*
 * Decimal and hexadecimal text to binary floating point (C23 7.24.1.5),
 * shared by strtof, strtod and strtold; docs/research/strtod.md.
 */

/* An IEEE 754 binary interchange format, and its exact fast path */
struct __libc_float_format {
	/* Significand bits including the implicit one (24, 53) */
	unsigned int precision;
	/* Exponent of the smallest and the largest normal binade */
	int emin;
	int emax;
	/* The largest power of ten the format holds exactly */
	int max_pow10;
	/* significand * 10^exponent10 in one correctly rounded operation, as format bits; significand <= 2^precision, |exponent10| <= max_pow10 */
	uint64_t (*exact)(uint64_t significand, int exponent10);
};

/*
 * Parse nptr as strtod does and return the bit pattern of the result in the
 * given format, correctly rounded in the given direction (an FLT_ROUNDS
 * value). Sets errno to ERANGE on overflow and underflow.
 */
uint64_t __libc_strtofloat(const char *restrict nptr, char **restrict endptr, const struct __libc_float_format *format, int rounding);

/*
 * The slow path for decimal input: digits (with at most one '.') in
 * [begin, end), times 10^exponent10; the result is unsigned.
 */
uint64_t __libc_decimal_to_float(const char *begin, const char *end, int exponent10, bool negative, const struct __libc_float_format *format, int rounding);

/*
 * Round significand * 2^exponent, plus a sticky bit for non-zero bits below
 * the significand, to the format in the given direction and return its bits
 * including the sign. Sets errno to ERANGE on overflow and underflow.
 */
uint64_t __libc_float_pack(bool negative, uint64_t significand, int exponent, bool sticky, const struct __libc_float_format *format, int rounding);

/* Infinity's bit pattern; the exponent field of infinity is emax - emin + 2 */
static inline uint64_t __libc_float_infinity(const struct __libc_float_format *format)
{
	return (uint64_t)(format->emax - format->emin + 2) << (format->precision - 1);
}

/* The sign bit sits just above the all-ones exponent field of infinity */
static inline uint64_t __libc_float_sign(const struct __libc_float_format *format)
{
	return __libc_float_infinity(format) + ((uint64_t)1 << (format->precision - 1));
}

#endif /* _INTERNAL_STRTOFLOAT_H */
