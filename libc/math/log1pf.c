/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * log1pf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.14: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float log1pf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x41078feb, 0x400fe5e7 }, /* 0x1.0f1fd6p+3 -> 0x1.1fcbcep+1 */
		{ 0x35400003, 0x353fffff }, /* 0x1.800006p-21 -> 0x1.7ffffep-21 */
		{ 0xb53ffffd, 0xb5400001 }, /* -0x1.7ffffap-21 -> -0x1.800002p-21 */
		{ 0x3710001b, 0x370ffff3 }, /* 0x1.200036p-17 -> 0x1.1fffe6p-17 */
		{ 0xb70fffe5, 0xb710000d }, /* -0x1.1fffcap-17 -> -0x1.20001ap-17 */
		{ 0xbb0ec8c4, 0xbb0ef0a5 }, /* -0x1.1d9188p-9 -> -0x1.1de14ap-9 */
		{ 0x3efd81ad, 0x3ecdeee1 }, /* 0x1.fb035ap-2 -> 0x1.9bddc2p-2 */
		{ 0x65d890d3, 0x4254d1f9 }, /* 0x1.b121a6p+76 -> 0x1.a9a3f2p+5 */
		{ 0x6f31a8ec, 0x42845a89 }, /* 0x1.6351d8p+95 -> 0x1.08b512p+6 */
	};

	if (isless(fabsf(x), 0x1p-30f))
		return __libc_math_tinyf(x, -1.0);

	return __libc_float_round(log1p(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
