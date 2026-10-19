/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_atomic_enter.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>

#include "rtos-none.h"

/* Masks interrupts (PRIMASK) and returns the previous mask: one core, so nothing else runs */
unsigned long __rtos_atomic_enter(void)
{
	unsigned long state;

	__asm__ volatile("mrs %0, primask\n\tcpsid i" : "=r"(state)::"memory");
	return state;
}
