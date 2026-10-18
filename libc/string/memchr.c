/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memchr.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "internal/word.h"

/* C23 7.26.5.2; parenthesised so the generic macro does not expand */
void *(memchr)(const void *s, int c, size_t n)
{
	const unsigned char *p = s;
	unsigned char byte = c;

	while (n > 0 && !__libc_word_aligned(p)) {
		if (*p == byte)
			return (void *)p;
		++p;
		--n;
	}

	/* Only whole words inside the object are read */
	if (n >= __LIBC_WORD_SIZE) {
		uintptr_t pattern = __libc_word_repeat(byte);

		while (n >= __LIBC_WORD_SIZE && !__libc_word_has_zero(__libc_word_load(p) ^ pattern)) {
			p += __LIBC_WORD_SIZE;
			n -= __LIBC_WORD_SIZE;
		}
	}

	while (n > 0) {
		if (*p == byte)
			return (void *)p;
		++p;
		--n;
	}

	return nullptr;
}
