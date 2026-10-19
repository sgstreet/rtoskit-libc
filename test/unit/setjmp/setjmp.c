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

/*
 * <setjmp.h> (P1.05): return values, volatile locals, jumps out of nested
 * calls, and the callee-saved registers (P1.06)
 */

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

/* Distinct run-time values the compiler cannot fold, and a call it cannot see through */
static volatile int seeds[8] = { 3, 4, 5, 6, 7, 8, 9, 10 };
static volatile double seeds_fp[8] = { 0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5 };
static void (*volatile jump)(int n, int val) = dive;

/*
 * Keep integer and floating-point values in callee-saved registers across a
 * call (so they are pushed in the prologue), then jump out from below,
 * skipping the epilogue that would restore the caller's values.
 */
[[gnu::noinline]] static int clobber(int n)
{
	int i0 = n * 3, i1 = n * 5, i2 = n * 7, i3 = n * 11, i4 = n * 13, i5 = n * 17, i6 = n * 19, i7 = n * 23;
	double d0 = n * 0.25, d1 = n * 0.75, d2 = n * 1.25, d3 = n * 1.75, d4 = n * 2.25, d5 = n * 2.75, d6 = n * 3.25, d7 = n * 3.75;

	jump(0, 1);

	return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + (int)(d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7);
}

/* The setjmp caller itself keeps nothing live across the setjmp */
[[gnu::noinline]] static int land(int n)
{
	if (setjmp(env) == 0)
		return clobber(n);
	return 0;
}

/*
 * The values here live across the call to land(), so they sit in
 * callee-saved registers that land() and clobber() may use; after the
 * longjmp they are intact only if longjmp restored those registers
 * (AAPCS r4-r11 and d8-d15, psABI rbx, rbp, r12-r15). Effective in optimised
 * builds, where the values are not kept on the stack.
 */
[[gnu::noinline]] static void check_callee_saved(void)
{
	int a = seeds[0], b = seeds[1], c = seeds[2], d = seeds[3], e = seeds[4], f = seeds[5], g = seeds[6], h = seeds[7];
	double p = seeds_fp[0], q = seeds_fp[1], r = seeds_fp[2], s = seeds_fp[3], t = seeds_fp[4], u = seeds_fp[5], v = seeds_fp[6], w = seeds_fp[7];

	CHECK(land(seeds[0] * 1000) == 0);
	CHECK(a == 3 && b == 4 && c == 5 && d == 6 && e == 7 && f == 8 && g == 9 && h == 10);
	CHECK(p == 0.5 && q == 1.5 && r == 2.5 && s == 3.5 && t == 4.5 && u == 5.5 && v == 6.5 && w == 7.5);
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

	check_callee_saved();

	static_assert(__STDC_VERSION_SETJMP_H__ == 202311L);

	if (failures == 0)
		puts("pass: setjmp");

	return failures == 0 ? 0 : 1;
}
