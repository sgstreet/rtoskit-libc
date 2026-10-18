/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strlen.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "internal/word.h"

/* C23 7.26.6.4 */
size_t strlen(const char *s)
{
	const char *p = s;

	while (!__libc_word_aligned(p)) {
		if (*p == '\0')
			return p - s;
		++p;
	}

	while (!__libc_word_has_zero(__libc_word_load(p)))
		p += __LIBC_WORD_SIZE;

	while (*p != '\0')
		++p;

	return p - s;
}
