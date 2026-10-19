/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_file_mode.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <stdio.h>

#include "internal/file.h"

/*
 * C23 7.23.5.3p3-p5: "r", "w" or "a", then in any order "+" (update), "b"
 * (binary, the same as text here) and, for "w" only, "x" (fail if the file
 * exists). Anything else is not a mode.
 */
bool __libc_file_mode(const char *mode, unsigned int *open_flags, unsigned short *stream_flags)
{
	unsigned int open = 0;
	bool update = false;
	bool binary = false;
	bool exclusive = false;

	switch (*mode++) {
		case 'r':
			open = __RTOS_OPEN_READ;
			break;
		case 'w':
			open = __RTOS_OPEN_WRITE | __RTOS_OPEN_CREATE | __RTOS_OPEN_TRUNCATE;
			break;
		case 'a':
			open = __RTOS_OPEN_WRITE | __RTOS_OPEN_CREATE | __RTOS_OPEN_APPEND;
			break;
		default:
			errno = EINVAL;
			return false;
	}

	for (; *mode != '\0'; ++mode) {
		if (*mode == '+' && !update)
			update = true;
		else if (*mode == 'b' && !binary)
			binary = true;
		else if (*mode == 'x' && !exclusive && (open & __RTOS_OPEN_TRUNCATE))
			exclusive = true;
		else {
			errno = EINVAL;
			return false;
		}
	}

	if (update)
		open |= __RTOS_OPEN_READ | __RTOS_OPEN_WRITE;
	if (exclusive)
		open |= __RTOS_OPEN_EXCLUSIVE;

	*open_flags = open;
	*stream_flags = ((open & __RTOS_OPEN_READ) ? __LIBC_FILE_READ : 0) | ((open & __RTOS_OPEN_WRITE) ? __LIBC_FILE_WRITE : 0);

	return true;
}
