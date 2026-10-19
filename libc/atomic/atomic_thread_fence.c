/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atomic_thread_fence.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>

/* C23 7.17.4.1; the parentheses bypass the macro */
void(atomic_thread_fence)(memory_order order)
{
	__atomic_thread_fence(order);
}
