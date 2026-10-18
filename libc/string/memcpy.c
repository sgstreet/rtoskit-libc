/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memcpy.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "internal/word.h"

/* C23 7.26.2.1 */
void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
	unsigned char *dst = s1;
	const unsigned char *src = s2;

	/* Whole words only when both share the same alignment phase */
	if ((((uintptr_t)dst ^ (uintptr_t)src) & (__LIBC_WORD_SIZE - 1)) == 0) {
		while (n > 0 && !__libc_word_aligned(dst)) {
			*dst++ = *src++;
			--n;
		}
		while (n >= __LIBC_WORD_SIZE) {
			__libc_word_store(dst, __libc_word_load(src));
			dst += __LIBC_WORD_SIZE;
			src += __LIBC_WORD_SIZE;
			n -= __LIBC_WORD_SIZE;
		}
	}

	while (n > 0) {
		*dst++ = *src++;
		--n;
	}

	return s1;
}
