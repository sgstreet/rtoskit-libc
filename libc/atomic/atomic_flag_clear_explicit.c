/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atomic_flag_clear_explicit.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>

/* C23 7.17.8.2 */
void(atomic_flag_clear_explicit)(volatile atomic_flag *object, memory_order order)
{
	__atomic_clear(&object->__value, order);
}
