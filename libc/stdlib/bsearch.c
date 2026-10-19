/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bsearch.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <stdlib.h>

/* C23 7.24.5.1; parenthesised so the generic macro does not expand */
void *(bsearch)(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	const unsigned char *low = base;

	/* The answer, if any, lies in [low, low + nmemb) */
	while (nmemb > 0) {
		const unsigned char *middle = low + (nmemb / 2) * size;
		int order = compar(key, middle);

		if (order == 0)
			return (void *)middle;
		if (order > 0) {
			low = middle + size;
			nmemb -= nmemb / 2 + 1;
		} else {
			nmemb /= 2;
		}
	}

	return nullptr;
}
