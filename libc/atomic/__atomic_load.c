/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_load.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/*
 * The __atomic_load library functions (GCC's library interface,
 * docs/research/threads.md). A naturally aligned load of up to 4 bytes is
 * single-copy atomic; 8 bytes and other sizes are read in the critical
 * section, which every other 8-byte or odd-sized operation also takes.
 */

#define DEFINE_LOAD_SMALL(size, type)                                                                       \
	type __libc_atomic_load_##size(const volatile void *object, int order) __asm__("__atomic_load_" #size); \
	type __libc_atomic_load_##size(const volatile void *object, [[maybe_unused]] int order)                 \
	{                                                                                                       \
		return *(const volatile type *)object;                                                              \
	}

DEFINE_LOAD_SMALL(1, uint8_t)
DEFINE_LOAD_SMALL(2, uint16_t)
DEFINE_LOAD_SMALL(4, uint32_t)

uint64_t __libc_atomic_load_8(const volatile void *object, int order) __asm__("__atomic_load_8");

uint64_t __libc_atomic_load_8(const volatile void *object, [[maybe_unused]] int order)
{
	unsigned long state = __rtos_atomic_enter();
	uint64_t value = *(const volatile uint64_t *)object;

	__rtos_atomic_exit(state);
	return value;
}

void __libc_atomic_load(size_t size, const volatile void *object, void *result, int order) __asm__("__atomic_load");

void __libc_atomic_load(size_t size, const volatile void *object, void *result, [[maybe_unused]] int order)
{
	unsigned long state = __rtos_atomic_enter();

	__libc_atomic_copy(result, object, size);
	__rtos_atomic_exit(state);
}
