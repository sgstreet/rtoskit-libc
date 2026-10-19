/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_none_set_tp.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "rtos-none.h"

/* The %fs base; the board must have enabled FSGSBASE (CR4) */
void __rtos_none_set_tp(void *tp)
{
	__asm__ volatile("wrfsbase %0" ::"r"(tp) : "memory");
}
