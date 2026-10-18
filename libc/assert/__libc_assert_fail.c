/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_assert_fail.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* C23 7.2.2.1: report the failed assertion on the standard error stream, then abort */
void __libc_assert_fail(const char *expression, const char *file, int line, const char *function)
{
	char message[256];

	snprintf(message, sizeof(message), "%s:%d: %s: Assertion '%s' failed.\n", file, line, function, expression);
	fputs(message, stderr);
	abort();
}
