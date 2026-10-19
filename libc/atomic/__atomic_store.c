/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_store.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/* The __atomic_store library functions: 1, 2 and 4 bytes through the word compare-and-swap, so a concurrent read-modify-write cannot lose them */

#define DEFINE_STORE_SMALL(size, type)                                                                              \
	void __libc_atomic_store_##size(volatile void *object, type value, int order) __asm__("__atomic_store_" #size); \
	void __libc_atomic_store_##size(volatile void *object, type value, [[maybe_unused]] int order)                  \
	{                                                                                                               \
		type current = *(volatile type *)object;                                                                    \
		type seen;                                                                                                  \
                                                                                                                    \
		while ((seen = (type)__libc_atomic_cas_small(object, size, current, value)) != current)                     \
			current = seen;                                                                                         \
	}

DEFINE_STORE_SMALL(1, uint8_t)
DEFINE_STORE_SMALL(2, uint16_t)
DEFINE_STORE_SMALL(4, uint32_t)

void __libc_atomic_store_8(volatile void *object, uint64_t value, int order) __asm__("__atomic_store_8");

void __libc_atomic_store_8(volatile void *object, uint64_t value, [[maybe_unused]] int order)
{
	unsigned long state = __rtos_atomic_enter();

	*(volatile uint64_t *)object = value;
	__rtos_atomic_exit(state);
}

void __libc_atomic_store(size_t size, volatile void *object, void *value, int order) __asm__("__atomic_store");

void __libc_atomic_store(size_t size, volatile void *object, void *value, [[maybe_unused]] int order)
{
	unsigned long state = __rtos_atomic_enter();

	__libc_atomic_copy(object, value, size);
	__rtos_atomic_exit(state);
}
