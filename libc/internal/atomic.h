/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atomic.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _INTERNAL_ATOMIC_H
#define _INTERNAL_ATOMIC_H

#include <rtos/rtos-port.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Helpers of the __atomic_* library functions (docs/research/threads.md).
 * Objects of 1, 2 and 4 bytes are updated with the port's word
 * compare-and-swap, which is atomic against the plain loads and stores the
 * compiler inlines for them (armv6-m inlines those but not the
 * read-modify-writes). 8-byte and odd-sized objects are only ever accessed
 * through the library, so a lock suffices: the port's critical section. The
 * functions are defined under C names with asm labels because the
 * __atomic_* names are compiler builtins.
 */

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "the byte lanes below assume a little-endian word");

/*
 * Compare-and-swap of a naturally aligned 1-, 2- or 4-byte object through
 * the word that holds it: stores desired if the object holds expected, and
 * returns the value it held. A change to another byte of the word only
 * makes it retry.
 */
static inline unsigned int __libc_atomic_cas_small(volatile void *object, size_t size, unsigned int expected, unsigned int desired)
{
	volatile unsigned int *word = (volatile unsigned int *)((uintptr_t)object & ~(uintptr_t)3);
	unsigned int lane = ((uintptr_t)object & 3) * 8;
	unsigned int mask = size == 4 ? 0xffffffffu : (1u << (size * 8)) - 1;

	while (true) {
		unsigned int current = *word;
		unsigned int value = (current >> lane) & mask;

		if (value != expected)
			return value;
		if (__rtos_atomic_cas(word, current, (current & ~(mask << lane)) | ((desired & mask) << lane)))
			return value;
	}
}

static inline void __libc_atomic_copy(volatile void *to, const volatile void *from, size_t size)
{
	volatile unsigned char *t = to;
	const volatile unsigned char *f = from;

	for (size_t i = 0; i < size; ++i)
		t[i] = f[i];
}

static inline bool __libc_atomic_equal(const volatile void *a, const volatile void *b, size_t size)
{
	const volatile unsigned char *x = a;
	const volatile unsigned char *y = b;

	for (size_t i = 0; i < size; ++i)
		if (x[i] != y[i])
			return false;

	return true;
}

#endif /* _INTERNAL_ATOMIC_H */
