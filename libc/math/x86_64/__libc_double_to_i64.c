/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_double_to_i64.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/math.h"

/* CVTTSD2SI converts 64 bits in one instruction, exactly for an integral x (PLAN D55) */
int64_t __libc_double_to_i64(double x)
{
	return (int64_t)x;
}
