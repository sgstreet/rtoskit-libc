/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_atexit_run.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "internal/stdlib.h"

/*
 * The atexit table lives with its runner: atexit references the table, which
 * links this object, and exit calls the runner through a weak reference, so a
 * program that never calls atexit carries neither (PLAN D52).
 */
struct __libc_exit_handlers __libc_atexit_handlers;

void __libc_atexit_run(void)
{
	__libc_exit_run(&__libc_atexit_handlers);
}
