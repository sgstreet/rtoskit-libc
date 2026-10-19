/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atomic_flag_test_and_set.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>

/* C23 7.17.8.1 */
bool(atomic_flag_test_and_set)(volatile atomic_flag *object)
{
	return __atomic_test_and_set(&object->__value, __ATOMIC_SEQ_CST);
}
