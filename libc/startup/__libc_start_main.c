/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_start_main.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <rtos/rtos-port.h>

#include "internal/sections.h"

static void run_array(void (*const *start)(void), void (*const *end)(void))
{
	for (void (*const *entry)(void) = start; entry < end; ++entry)
		(*entry)();
}

void __libc_start_main(int (*main)(int, char **), int argc, char **argv)
{
	run_array(__preinit_array_start, __preinit_array_end);
	run_array(__init_array_start, __init_array_end);

	exit(main(argc, argv));
}
