/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * perror.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "internal/file.h"

/* C23 7.23.10.4: "s: message\n", or just the message for a null or empty s, as one write sequence */
void perror(const char *s)
{
	const char *message = strerror(errno);

	__libc_file_lock(stderr);
	if (s != nullptr && *s != '\0') {
		__libc_file_write(stderr, s, strlen(s));
		__libc_file_write(stderr, ": ", 2);
	}
	__libc_file_write(stderr, message, strlen(message));
	__libc_file_write(stderr, "\n", 1);
	__libc_file_unlock(stderr);
}
