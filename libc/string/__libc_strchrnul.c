/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_strchrnul.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>

#include "internal/string.h"
#include "internal/word.h"

/* Word-at-a-time core of strchr, strcspn and friends */
char *__libc_strchrnul(const char *s, int c)
{
	const unsigned char *p = (const unsigned char *)s;
	unsigned char byte = c;
	uintptr_t pattern;

	while (!__libc_word_aligned(p)) {
		if (*p == byte || *p == '\0')
			return (char *)p;
		++p;
	}

	/* Stop at the first word holding the terminator or the byte */
	pattern = __libc_word_repeat(byte);
	while (true) {
		uintptr_t word = __libc_word_load(p);

		if (__libc_word_has_zero(word) || __libc_word_has_zero(word ^ pattern))
			break;
		p += __LIBC_WORD_SIZE;
	}

	while (*p != byte && *p != '\0')
		++p;

	return (char *)p;
}
