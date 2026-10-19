/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fread.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdckdint.h>
#include <stdio.h>

#include "internal/file.h"

/* C23 7.23.8.1: the number of complete elements read */
size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
	size_t total;
	size_t done;

	if (size == 0 || nmemb == 0)
		return 0;
	/* More than fits in memory: read as many elements as SIZE_MAX bytes hold */
	if (ckd_mul(&total, size, nmemb))
		total = (SIZE_MAX / size) * size;

	__libc_file_before_input(stream);
	__libc_file_lock(stream);
	done = __libc_file_read(stream, ptr, total);
	__libc_file_unlock(stream);

	return done / size;
}
