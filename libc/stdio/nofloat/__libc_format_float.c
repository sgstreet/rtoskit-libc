/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_float.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>

#include "internal/format.h"

/*
 * libc-nofloat.a: a/A/e/E/f/F/g/G are invalid conversion specifications, which
 * fail with EINVAL as any other does (D28). The full formatter, the decimal
 * module and the soft-float helpers they need stay out of the program (PLAN
 * D13, D53).
 */
int __libc_format_float([[maybe_unused]] struct __libc_format_sink *sink, [[maybe_unused]] const struct __libc_format_spec *spec, [[maybe_unused]] long double value, [[maybe_unused]] size_t count)
{
	errno = EINVAL;
	return -1;
}
