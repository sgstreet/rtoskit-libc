/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fgets.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>

#include "internal/file.h"

/*
 * C23 7.23.7.2: at most n - 1 characters, through a new-line. Null at end of
 * file before any character (s unchanged) and on a read error.
 */
char *fgets(char *restrict s, int n, FILE *restrict stream)
{
	char *result = s;
	int count = 0;

	if (n <= 0)
		return nullptr;

	__libc_file_before_input(stream);
	__libc_file_lock(stream);
	while (count < n - 1) {
		int c = __libc_file_getc(stream);

		if (c == EOF) {
			if (count == 0 || (stream->flags & __LIBC_FILE_ERROR))
				result = nullptr;
			break;
		}
		s[count++] = c;
		if (c == '\n')
			break;
	}
	__libc_file_unlock(stream);

	if (result != nullptr)
		s[count] = '\0';

	return result;
}
