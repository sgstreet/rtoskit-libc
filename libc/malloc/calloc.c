/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * calloc.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdckdint.h>
#include <stdlib.h>
#include <string.h>

/* C23 7.24.3.2: fail when nmemb * size wraps around */
void *calloc(size_t nmemb, size_t size)
{
	size_t total;
	void *p;

	if (ckd_mul(&total, nmemb, size)) {
		errno = ENOMEM;
		return nullptr;
	}

	p = malloc(total);
	if (p != nullptr)
		memset(p, 0, total);

	return p;
}
