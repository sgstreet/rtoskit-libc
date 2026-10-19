/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_test_and_set.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/atomic.h"

/* The library form of __atomic_test_and_set (armv6-m): set a byte, report whether it was set */
bool __libc_atomic_test_and_set(volatile void *object, int order) __asm__("__atomic_test_and_set");

bool __libc_atomic_test_and_set(volatile void *object, [[maybe_unused]] int order)
{
	unsigned int current = *(volatile unsigned char *)object;
	unsigned int seen;

	while ((seen = __libc_atomic_cas_small(object, 1, current, 1)) != current)
		current = seen;

	return current != 0;
}
