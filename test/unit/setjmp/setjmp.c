/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setjmp.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <setjmp.h>
#include <stdio.h>

/* <setjmp.h> (P1.05): return values, volatile locals, and jumps out of nested calls */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static jmp_buf env;
static int depth;

/* Recurse n levels, then jump back to the setjmp; a negative n returns normally */
[[gnu::noinline]] static void dive(int n, int val)
{
	if (n < 0)
		return;

	++depth;
	if (n == 0)
		longjmp(env, val);
	dive(n - 1, val);
}

int main(void)
{
	volatile int count = 0;
	int status;

	status = setjmp(env);
	if (status == 0) {
		count = 1;
		dive(10, 0);
	}
	/* C23 7.13.2.1p4: longjmp(env, 0) makes setjmp return 1 */
	CHECK(status == 1 && count == 1 && depth == 11);

	status = setjmp(env);
	if (status == 0)
		dive(3, 42);
	CHECK(status == 42);

	count = 0;
	status = setjmp(env);
	if (++count < 5)
		longjmp(env, count);
	CHECK(count == 5 && status == 4);

	static_assert(__STDC_VERSION_SETJMP_H__ == 202311L);

	if (failures == 0)
		puts("pass: setjmp");

	return failures == 0 ? 0 : 1;
}
