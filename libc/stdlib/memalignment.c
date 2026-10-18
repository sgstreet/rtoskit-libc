/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memalignment.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <stdlib.h>

/* C23 7.24.9.1: the largest power of two dividing the address; zero for a null pointer */
size_t memalignment(const void *p)
{
	uintptr_t address = (uintptr_t)p;

	return address & -address;
}
