/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memset.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "internal/word.h"

/* C23 7.26.6.1 */
void *memset(void *s, int c, size_t n)
{
	unsigned char *dst = s;
	unsigned char byte = c;

	while (n > 0 && !__libc_word_aligned(dst)) {
		*dst++ = byte;
		--n;
	}

	if (n >= __LIBC_WORD_SIZE) {
		uintptr_t word = __libc_word_repeat(byte);

		while (n >= __LIBC_WORD_SIZE) {
			__libc_word_store(dst, word);
			dst += __LIBC_WORD_SIZE;
			n -= __LIBC_WORD_SIZE;
		}
	}

	while (n > 0) {
		*dst++ = byte;
		--n;
	}

	return s;
}
