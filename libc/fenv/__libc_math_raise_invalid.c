/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_math_raise_invalid.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "fenv-arch.h"
#include "internal/math.h"

/* 0.0f / 0.0f, which is invalid; nothing where the core records no exceptions (PLAN D55) */
void __libc_math_raise_invalid(void)
{
	if (__libc_fenv_records) {
		volatile float zero = 0.0f;
		[[maybe_unused]] volatile float result = zero / zero;
	}
}
