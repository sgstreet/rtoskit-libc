/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_compare_exchange.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/*
 * The __atomic_compare_exchange library functions. The library interface has
 * no weak argument: these never fail spuriously.
 */

#define DEFINE_COMPARE_EXCHANGE_SMALL(size, type)                                                                                                                          \
	bool __libc_atomic_compare_exchange_##size(volatile void *object, void *expected, type desired, int success, int failure) __asm__("__atomic_compare_exchange_" #size); \
	bool __libc_atomic_compare_exchange_##size(volatile void *object, void *expected, type desired, [[maybe_unused]] int success, [[maybe_unused]] int failure)            \
	{                                                                                                                                                                      \
		type wanted = *(type *)expected;                                                                                                                                   \
		type seen = (type)__libc_atomic_cas_small(object, size, wanted, desired);                                                                                          \
                                                                                                                                                                           \
		if (seen == wanted)                                                                                                                                                \
			return true;                                                                                                                                                   \
		*(type *)expected = seen;                                                                                                                                          \
		return false;                                                                                                                                                      \
	}

DEFINE_COMPARE_EXCHANGE_SMALL(1, uint8_t)
DEFINE_COMPARE_EXCHANGE_SMALL(2, uint16_t)
DEFINE_COMPARE_EXCHANGE_SMALL(4, uint32_t)

bool __libc_atomic_compare_exchange_8(volatile void *object, void *expected, uint64_t desired, int success, int failure) __asm__("__atomic_compare_exchange_8");

bool __libc_atomic_compare_exchange_8(volatile void *object, void *expected, uint64_t desired, [[maybe_unused]] int success, [[maybe_unused]] int failure)
{
	unsigned long state = __rtos_atomic_enter();
	uint64_t current = *(volatile uint64_t *)object;
	bool equal = current == *(uint64_t *)expected;

	if (equal)
		*(volatile uint64_t *)object = desired;
	else
		*(uint64_t *)expected = current;
	__rtos_atomic_exit(state);

	return equal;
}

bool __libc_atomic_compare_exchange(size_t size, volatile void *object, void *expected, void *desired, int success, int failure) __asm__("__atomic_compare_exchange");

/* As if by memcmp and memcpy (C23 7.17.7.4p2) */
bool __libc_atomic_compare_exchange(size_t size, volatile void *object, void *expected, void *desired, [[maybe_unused]] int success, [[maybe_unused]] int failure)
{
	unsigned long state = __rtos_atomic_enter();
	bool equal = __libc_atomic_equal(object, expected, size);

	if (equal)
		__libc_atomic_copy(object, desired, size);
	else
		__libc_atomic_copy(expected, object, size);
	__rtos_atomic_exit(state);

	return equal;
}
