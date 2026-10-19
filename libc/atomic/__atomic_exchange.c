/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_exchange.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/* The __atomic_exchange library functions */

#define DEFINE_EXCHANGE_SMALL(size, type)                                                                                 \
	type __libc_atomic_exchange_##size(volatile void *object, type value, int order) __asm__("__atomic_exchange_" #size); \
	type __libc_atomic_exchange_##size(volatile void *object, type value, [[maybe_unused]] int order)                     \
	{                                                                                                                     \
		type current = *(volatile type *)object;                                                                          \
		type seen;                                                                                                        \
                                                                                                                          \
		while ((seen = (type)__libc_atomic_cas_small(object, size, current, value)) != current)                           \
			current = seen;                                                                                               \
		return current;                                                                                                   \
	}

DEFINE_EXCHANGE_SMALL(1, uint8_t)
DEFINE_EXCHANGE_SMALL(2, uint16_t)
DEFINE_EXCHANGE_SMALL(4, uint32_t)

uint64_t __libc_atomic_exchange_8(volatile void *object, uint64_t value, int order) __asm__("__atomic_exchange_8");

uint64_t __libc_atomic_exchange_8(volatile void *object, uint64_t value, [[maybe_unused]] int order)
{
	unsigned long state = __rtos_atomic_enter();
	uint64_t previous = *(volatile uint64_t *)object;

	*(volatile uint64_t *)object = value;
	__rtos_atomic_exit(state);
	return previous;
}

void __libc_atomic_exchange(size_t size, volatile void *object, void *value, void *result, int order) __asm__("__atomic_exchange");

/* Byte by byte, so value and result may be the same object */
void __libc_atomic_exchange(size_t size, volatile void *object, void *value, void *result, [[maybe_unused]] int order)
{
	volatile unsigned char *target = object;
	unsigned char *source = value;
	unsigned char *previous = result;
	unsigned long state = __rtos_atomic_enter();

	for (size_t i = 0; i < size; ++i) {
		unsigned char byte = target[i];

		target[i] = source[i];
		previous[i] = byte;
	}
	__rtos_atomic_exit(state);
}
