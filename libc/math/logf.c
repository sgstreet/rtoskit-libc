/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * logf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.6.11: evaluated in double, then rounded once more (docs/research/libm-exp-log.md) */
float logf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x41178feb, 0x400fe5e7 }, /* 0x1.2f1fd6p+3 -> 0x1.1fcbcep+1 */
		{ 0x4c5d65a5, 0x418f034b }, /* 0x1.bacb4ap+25 -> 0x1.1e0696p+4 */
		{ 0x65d890d3, 0x4254d1f9 }, /* 0x1.b121a6p+76 -> 0x1.a9a3f2p+5 */
		{ 0x6f31a8ec, 0x42845a89 }, /* 0x1.6351d8p+95 -> 0x1.08b512p+6 */
		{ 0x3c413d3a, 0xc08e158f }, /* 0x1.827a74p-7 -> -0x1.1c2b1ep+2 */
	};

	return __libc_float_round(log(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
