/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtod.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include "internal/strtofloat.h"
#include "internal/math.h"

/* 10^0 .. 10^22 are exact in binary64 */
static const double powers_of_ten[] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22 };

static uint64_t exact(uint64_t significand, int exponent10)
{
	double value = __libc_u64_to_double(significand);
	uint64_t bits;

	value = exponent10 < 0 ? value / powers_of_ten[-exponent10] : value * powers_of_ten[exponent10];
	__builtin_memcpy(&bits, &value, sizeof(bits));

	return bits;
}

static const struct __libc_float_format binary64 = {
	.precision = DBL_MANT_DIG,
	.emin = DBL_MIN_EXP - 1,
	.emax = DBL_MAX_EXP - 1,
	.max_pow10 = sizeof(powers_of_ten) / sizeof(powers_of_ten[0]) - 1,
	.exact = exact,
};

static_assert(sizeof(double) == sizeof(uint64_t) && DBL_MANT_DIG == 53);

/* C23 7.24.1.5, correctly rounded in the current rounding direction (F.5) */
double strtod(const char *restrict nptr, char **restrict endptr)
{
	uint64_t bits = __libc_strtofloat(nptr, endptr, &binary64, FLT_ROUNDS);
	double value;

	__builtin_memcpy(&value, &bits, sizeof(value));

	return value;
}
