/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strtof.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include "internal/strtofloat.h"

/* 10^0 .. 10^10 are exact in binary32 */
static const float powers_of_ten[] = { 1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f, 1e7f, 1e8f, 1e9f, 1e10f };

/* Computed in float, never through double, so there is a single rounding */
static uint64_t exact(uint64_t significand, int exponent10)
{
	float value = (float)significand;
	uint32_t bits;

	value = exponent10 < 0 ? value / powers_of_ten[-exponent10] : value * powers_of_ten[exponent10];
	__builtin_memcpy(&bits, &value, sizeof(bits));

	return bits;
}

static const struct __libc_float_format binary32 = {
	.precision = FLT_MANT_DIG,
	.emin = FLT_MIN_EXP - 1,
	.emax = FLT_MAX_EXP - 1,
	.max_pow10 = sizeof(powers_of_ten) / sizeof(powers_of_ten[0]) - 1,
	.exact = exact,
};

static_assert(sizeof(float) == sizeof(uint32_t) && FLT_MANT_DIG == 24);

/* C23 7.24.1.5, correctly rounded in the current rounding direction (F.5) */
float strtof(const char *restrict nptr, char **restrict endptr)
{
	uint32_t bits = (uint32_t)__libc_strtofloat(nptr, endptr, &binary32, FLT_ROUNDS);
	float value;

	__builtin_memcpy(&value, &bits, sizeof(value));

	return value;
}
