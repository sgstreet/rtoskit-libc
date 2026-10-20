/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_count.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>

#include "internal/format.h"

/* libc-minimal-printf.a leaves out %n: an invalid conversion specification, failing with EINVAL (PLAN D53) */
bool __libc_format_count([[maybe_unused]] va_list *ap, [[maybe_unused]] enum __libc_length length, [[maybe_unused]] size_t count)
{
	errno = EINVAL;
	return false;
}
