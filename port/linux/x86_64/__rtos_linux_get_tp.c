/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_linux_get_tp.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "linux.h"

/* The first word of the x86_64 thread control block points to itself */
void *__rtos_linux_get_tp(void)
{
	void *tp;

	__asm__ volatile("mov %%fs:0, %0" : "=r"(tp));
	return tp;
}
