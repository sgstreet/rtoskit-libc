/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ftell.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.9.4 */
long int ftell(FILE *stream)
{
	long long position;

	__libc_file_lock(stream);
	position = __libc_file_position(stream);
	__libc_file_unlock(stream);

	if (position > LONG_MAX) {
		errno = EOVERFLOW;
		return -1;
	}

	return (long int)position;
}
