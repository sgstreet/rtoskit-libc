/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * t_memfill.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

#include "test.h"

/*
 * libc-test's t_memfill exhausts the address space with mmap and limits brk.
 * Our heap is a fixed set of regions (D31), so allocating until malloc fails,
 * from large blocks down to the smallest, uses it all up the same way.
 */
int t_memfill(void)
{
	for (size_t size = (size_t)1 << 24; size > 0; size /= 2)
		while (malloc(size) != nullptr);

	return 0;
}
