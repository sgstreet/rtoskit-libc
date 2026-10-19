/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * quick_exit.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

#include "internal/stdlib.h"

/* C23 7.24.4.7: the at_quick_exit handlers, newest first, then _Exit */
void quick_exit(int status)
{
	__libc_exit_run(&__libc_quick_exit_handlers);
	_Exit(status);
}
