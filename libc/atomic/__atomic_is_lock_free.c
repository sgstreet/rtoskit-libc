/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __atomic_is_lock_free.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdint.h>

#include "internal/atomic.h"

/*
 * Called for the sizes the compiler cannot answer itself. Up to 4 bytes the
 * library uses the port's compare-and-swap; 8 bytes and odd sizes use a
 * lock (the port's critical section).
 */
bool __libc_atomic_is_lock_free(size_t size, const volatile void *object) __asm__("__atomic_is_lock_free");

bool __libc_atomic_is_lock_free(size_t size, const volatile void *object)
{
	return (size == 1 || size == 2 || size == 4) && ((uintptr_t)object & (size - 1)) == 0;
}
