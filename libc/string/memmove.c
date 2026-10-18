/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memmove.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <string.h>

#include "internal/word.h"

/* C23 7.26.2.3: copy as if through a temporary, so pick the direction that never overwrites unread source */
void *memmove(void *s1, const void *s2, size_t n)
{
	unsigned char *dst = s1;
	const unsigned char *src = s2;
	bool words = (((uintptr_t)dst ^ (uintptr_t)src) & (__LIBC_WORD_SIZE - 1)) == 0;

	if ((uintptr_t)dst - (uintptr_t)src >= n) {
		/* The destination starts before the source or past its end: copy forwards */
		if (words) {
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

	/* The destination overlaps the end of the source: copy backwards */
	dst += n;
	src += n;
	if (words) {
		while (n > 0 && !__libc_word_aligned(dst)) {
			*--dst = *--src;
			--n;
		}
		while (n >= __LIBC_WORD_SIZE) {
			dst -= __LIBC_WORD_SIZE;
			src -= __LIBC_WORD_SIZE;
			n -= __LIBC_WORD_SIZE;
			__libc_word_store(dst, __libc_word_load(src));
		}
	}
	while (n > 0) {
		*--dst = *--src;
		--n;
	}

	return s1;
}
