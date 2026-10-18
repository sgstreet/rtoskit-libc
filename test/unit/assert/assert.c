/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * assert.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/* Release builds pass -DNDEBUG; this test needs live assertions first */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
 * <assert.h> (P1.03): a true assertion does nothing and evaluates its
 * argument once; a false one reports on stderr and aborts (assert.run checks
 * the status and the message); NDEBUG is honoured on every inclusion.
 */

static int evaluations;

static int count(int value)
{
	++evaluations;
	return value;
}

static void with_ndebug(void);

int main(int argc, char **argv)
{
	int x = 1;
	struct pair {
		int a;
		int b;
	};

	assert(count(1));
	if (evaluations != 1)
		return 2;

	/* C23: the ellipsis lets a compound literal with commas through */
	assert((struct pair){ 1, 2 }.b == 2);
	assert(&x != nullptr);
	assert(1.5);

	with_ndebug();
	if (evaluations != 1)
		return 3;

	if (argc > 1 && strcmp(argv[1], "fail") == 0)
		assert(x == 2);

	puts("pass: assert");
	return 0;
}

#define NDEBUG
#include <assert.h>

/* With NDEBUG the argument is not evaluated */
static void with_ndebug(void)
{
	assert(count(0));
}
