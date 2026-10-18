/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * word.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_WORD_H
#define _INTERNAL_WORD_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Word-at-a-time helpers for the string functions (see
 * docs/research/string-ctype-stdbit.md). A word is a uintptr_t, the natural
 * register width. Words are only read at aligned addresses, so a read never
 * crosses a page or MPU region boundary even when it covers bytes past the
 * end of the object.
 */

#define __LIBC_WORD_SIZE sizeof(uintptr_t)
#define __LIBC_WORD_ONES (UINTPTR_MAX / UCHAR_MAX)
#define __LIBC_WORD_HIGHS (__LIBC_WORD_ONES * (UCHAR_MAX / 2 + 1))

static inline bool __libc_word_aligned(const void *p)
{
	return ((uintptr_t)p & (__LIBC_WORD_SIZE - 1)) == 0;
}

/* Aliasing-safe aligned load; the explicit builtin is expanded even under -fno-builtin */
static inline uintptr_t __libc_word_load(const void *p)
{
	uintptr_t word;

	__builtin_memcpy(&word, __builtin_assume_aligned(p, __LIBC_WORD_SIZE), sizeof(word));
	return word;
}

static inline void __libc_word_store(void *p, uintptr_t word)
{
	__builtin_memcpy(__builtin_assume_aligned(p, __LIBC_WORD_SIZE), &word, sizeof(word));
}

/* The byte repeated in every byte of a word */
static inline uintptr_t __libc_word_repeat(unsigned char c)
{
	return __LIBC_WORD_ONES * c;
}

/* Non-zero iff some byte of word is zero */
static inline bool __libc_word_has_zero(uintptr_t word)
{
	return ((word - __LIBC_WORD_ONES) & ~word & __LIBC_WORD_HIGHS) != 0;
}

#endif /* _INTERNAL_WORD_H */
