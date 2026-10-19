/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * malloc-heavy.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <string.h>

/* Reference program: the allocator without stdio; the result is the exit status */
int main(int argc, char **argv)
{
	char *blocks[16];
	size_t sum = 0;
	char *name;
	int *table;

	for (size_t i = 0; i < 16; ++i)
		blocks[i] = malloc(((size_t)argc + i) * 24);

	for (size_t i = 0; i < 16; i += 2) {
		free(blocks[i]);
		blocks[i] = nullptr;
	}

	table = calloc((size_t)argc + 7, sizeof(*table));
	if (table == nullptr)
		return EXIT_FAILURE;

	for (size_t i = 1; i < 16; i += 2) {
		char *bigger = realloc(blocks[i], ((size_t)argc + i) * 96);

		if (bigger == nullptr)
			return EXIT_FAILURE;
		blocks[i] = bigger;
		sum += (size_t)table[i % 8] + i;
	}

	name = strdup(argv[0]);
	if (name == nullptr)
		return EXIT_FAILURE;
	sum += strlen(name);

	free(name);
	free(table);
	for (size_t i = 0; i < 16; ++i)
		free(blocks[i]);

	return sum == 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
