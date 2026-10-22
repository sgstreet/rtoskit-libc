/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * tgammaf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/math.h"

/* C23 7.12.8.4: evaluated in double, then rounded once more (docs/research/libm-special.md) */
float tgammaf(float x)
{
	/* The arguments whose double result is a float halfway point while the exact value is not (PLAN D98) */
	static const struct __libc_float_case cases[] = {
		{ 0x27de86a9, 0x57134133 }, /* 0x1.bd0d52p-48 -> 0x1.268266p+47 */
		{ 0x27e05475, 0x57121211 }, /* 0x1.c0a8eap-48 -> 0x1.242422p+47 */
	};

	return __libc_float_round(tgamma(x), x, cases, sizeof(cases) / sizeof(cases[0]));
}
