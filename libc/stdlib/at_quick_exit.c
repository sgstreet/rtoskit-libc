/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * at_quick_exit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

#include "internal/stdlib.h"

/* C23 7.24.4.3 */
int at_quick_exit(void (*func)(void))
{
	return __libc_exit_register(&__libc_quick_exit_handlers, func);
}
