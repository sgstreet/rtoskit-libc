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

void *__rtos_none_tp;

/* __aeabi_read_tp reads it back; one thread needs no register */
void __rtos_none_set_tp(void *tp)
{
	__rtos_none_tp = tp;
}
