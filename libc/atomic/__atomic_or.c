/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_or.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/* The __atomic_fetch_or and __atomic_or_fetch library functions: the value before and after */

#define DEFINE_OR_SMALL(size, type)                                                                                         \
	static type or_##size(volatile void *object, type operand, bool after)                                                  \
	{                                                                                                                       \
		type previous = *(volatile type *)object;                                                                           \
		type next;                                                                                                          \
		type seen;                                                                                                          \
                                                                                                                            \
		while (true) {                                                                                                      \
			next = (type)(previous | operand);                                                                              \
			seen = (type)__libc_atomic_cas_small(object, size, previous, next);                                             \
			if (seen == previous)                                                                                           \
				return after ? next : previous;                                                                             \
			previous = seen;                                                                                                \
		}                                                                                                                   \
	}                                                                                                                       \
                                                                                                                            \
	type __libc_atomic_fetch_or_##size(volatile void *object, type operand, int order) __asm__("__atomic_fetch_or_" #size); \
	type __libc_atomic_fetch_or_##size(volatile void *object, type operand, [[maybe_unused]] int order)                     \
	{                                                                                                                       \
		return or_##size(object, operand, false);                                                                           \
	}                                                                                                                       \
                                                                                                                            \
	type __libc_atomic_or_fetch_##size(volatile void *object, type operand, int order) __asm__("__atomic_or_fetch_" #size); \
	type __libc_atomic_or_fetch_##size(volatile void *object, type operand, [[maybe_unused]] int order)                     \
	{                                                                                                                       \
		return or_##size(object, operand, true);                                                                            \
	}

DEFINE_OR_SMALL(1, uint8_t)
DEFINE_OR_SMALL(2, uint16_t)
DEFINE_OR_SMALL(4, uint32_t)

static uint64_t or_8(volatile void *object, uint64_t operand, bool after)
{
	unsigned long state = __rtos_atomic_enter();
	uint64_t previous = *(volatile uint64_t *)object;
	uint64_t next = previous | operand;

	*(volatile uint64_t *)object = next;
	__rtos_atomic_exit(state);

	return after ? next : previous;
}

uint64_t __libc_atomic_fetch_or_8(volatile void *object, uint64_t operand, int order) __asm__("__atomic_fetch_or_8");

uint64_t __libc_atomic_fetch_or_8(volatile void *object, uint64_t operand, [[maybe_unused]] int order)
{
	return or_8(object, operand, false);
}

uint64_t __libc_atomic_or_fetch_8(volatile void *object, uint64_t operand, int order) __asm__("__atomic_or_fetch_8");

uint64_t __libc_atomic_or_fetch_8(volatile void *object, uint64_t operand, [[maybe_unused]] int order)
{
	return or_8(object, operand, true);
}
