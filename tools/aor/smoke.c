/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * smoke.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * The Arm Optimized Routines smoke test (PLAN D92): tools/scripts/aor-build
 * links this with the AOR archives ahead of libc.a, checks in the map that
 * AOR's definitions were taken, and runs it. One call per substituted
 * routine with a known result: a link and run check, not an accuracy test
 * (that is P4.05). Operands go through volatile so that the compiler
 * neither folds the operations nor replaces the calls.
 */

static int failures;

static void check(const char *name, bool ok)
{
	if (!ok) {
		printf("FAIL %s\n", name);
		++failures;
	}
}

static bool near(double value, double expected, double tolerance)
{
	return fabs(value - expected) <= tolerance * fabs(expected);
}

static void test_string(void)
{
	static char buffer[80];
	static const char text[] = "the quick brown fox jumps over the lazy dog, 0123456789 abcdefghijklmnopqrstuv";
	char *volatile source = (char *)text;

	check("memchr found", memchr(source, 'z', sizeof(text)) == source + 37);
	check("memchr not found", memchr(source, '#', sizeof(text)) == nullptr);
	memset(buffer + 1, 'x', 33);
	check("memset", buffer[0] == 0 && buffer[1] == 'x' && buffer[33] == 'x' && buffer[34] == 0);
	memset(buffer + 1, 'y', 0);
	check("memset 0", buffer[1] == 'x');
	check("strlen 0", strlen(source + sizeof(text) - 1) == 0);
	check("strlen 1", strlen(source + sizeof(text) - 2) == 1);
	check("strlen 15", strlen(source + sizeof(text) - 16) == 15);
	check("strlen 64", strlen(source + sizeof(text) - 65) == 64);
	check("strcmp equal", strcmp(source, text) == 0);
	check("strcmp byte 0", strcmp(source, "uhe") < 0);
	check("strcmp byte 13", strcmp(source, "the quick broxn") < 0);
}

static void test_math(void)
{
	volatile float one = 1.0f;
	volatile float two = 2.0f;
	volatile float half = 0.5f;
	volatile double done = 1.0;
	volatile double dtwo = 2.0;
	volatile double dhalf = 0.5;

	check("expf", near(expf(one), 2.718281828459045, 0x1p-22));
	check("logf", near(logf(two), 0.6931471805599453, 0x1p-22));
	check("sinf", near(sinf(one), 0.8414709848078965, 0x1p-22));
	check("powf", near(powf(two, half), 1.4142135623730951, 0x1p-22));
	check("exp", near(exp(done), 2.718281828459045, 0x1p-51));
	check("log", near(log(dtwo), 0.6931471805599453, 0x1p-51));
	check("pow", near(pow(dtwo, dhalf), 1.4142135623730951, 0x1p-51));
}

static void test_soft_float(void)
{
	volatile float a = 1.5f;
	volatile float b = 3.0f;
	volatile double c = 1.0;
	volatile double d = 3.0;
	volatile double e = -1.5;

	check("fmul", a * b == 4.5f);
	check("fadd", a + b == 4.5f);
	check("fcmplt", a < b && !(b < a));
	check("ddiv", near(c / d, 0.3333333333333333, 0x1p-52));
	check("d2lz", (long long)e == -1);
}

int main(void)
{
	test_string();
	test_math();
	test_soft_float();
	if (failures)
		return 1;
	printf("aor-smoke: ok\n");
	return 0;
}
