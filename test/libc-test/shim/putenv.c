/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * putenv.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <string.h>

#include "test.h"

/* POSIX putenv: the string itself becomes the entry */
int putenv(char *string)
{
	char *equals = strchr(string, '=');

	if (equals == nullptr || equals == string) {
		errno = EINVAL;
		return -1;
	}

	return t_setenv(string, equals - string, 1);
}
