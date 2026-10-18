/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_float.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>

#include "internal/format.h"

/* Floating-point conversions arrive in P1.08; until then they fail */
int __libc_format_float([[maybe_unused]] struct __libc_format_sink *sink, [[maybe_unused]] const struct __libc_format_spec *spec, [[maybe_unused]] long double value)
{
	errno = EINVAL;
	return -1;
}
