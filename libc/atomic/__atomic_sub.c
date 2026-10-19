/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_sub.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/* The __atomic_fetch_sub and __atomic_sub_fetch library functions: the value before and after */

#define DEFINE_SUB_SMALL(size, type)                                                                                          \
	static type sub_##size(volatile void *object, type operand, bool after)                                                   \
	{                                                                                                                         \
		type previous = *(volatile type *)object;                                                                             \
		type next;                                                                                                            \
		type seen;                                                                                                            \
                                                                                                                              \
		while (true) {                                                                                                        \
			next = (type)(previous - operand);                                                                                \
			seen = (type)__libc_atomic_cas_small(object, size, previous, next);                                               \
			if (seen == previous)                                                                                             \
				return after ? next : previous;                                                                               \
			previous = seen;                                                                                                  \
		}                                                                                                                     \
	}                                                                                                                         \
                                                                                                                              \
	type __libc_atomic_fetch_sub_##size(volatile void *object, type operand, int order) __asm__("__atomic_fetch_sub_" #size); \
	type __libc_atomic_fetch_sub_##size(volatile void *object, type operand, [[maybe_unused]] int order)                      \
	{                                                                                                                         \
		return sub_##size(object, operand, false);                                                                            \
	}                                                                                                                         \
                                                                                                                              \
	type __libc_atomic_sub_fetch_##size(volatile void *object, type operand, int order) __asm__("__atomic_sub_fetch_" #size); \
	type __libc_atomic_sub_fetch_##size(volatile void *object, type operand, [[maybe_unused]] int order)                      \
	{                                                                                                                         \
		return sub_##size(object, operand, true);                                                                             \
	}

DEFINE_SUB_SMALL(1, uint8_t)
DEFINE_SUB_SMALL(2, uint16_t)
DEFINE_SUB_SMALL(4, uint32_t)

static uint64_t sub_8(volatile void *object, uint64_t operand, bool after)
{
	unsigned long state = __rtos_atomic_enter();
	uint64_t previous = *(volatile uint64_t *)object;
	uint64_t next = previous - operand;

	*(volatile uint64_t *)object = next;
	__rtos_atomic_exit(state);

	return after ? next : previous;
}

uint64_t __libc_atomic_fetch_sub_8(volatile void *object, uint64_t operand, int order) __asm__("__atomic_fetch_sub_8");

uint64_t __libc_atomic_fetch_sub_8(volatile void *object, uint64_t operand, [[maybe_unused]] int order)
{
	return sub_8(object, operand, false);
}

uint64_t __libc_atomic_sub_fetch_8(volatile void *object, uint64_t operand, int order) __asm__("__atomic_sub_fetch_8");

uint64_t __libc_atomic_sub_fetch_8(volatile void *object, uint64_t operand, [[maybe_unused]] int order)
{
	return sub_8(object, operand, true);
}
