/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strto.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

#include "bench.h"

/*
 * Numeric conversions (P3.01, docs/research/benchmarks.md): strtol and its
 * relatives on short, long, negative and hexadecimal input, and strtod and
 * strtof on the input classes that take different paths in a decimal to
 * binary converter: short decimals, 17 significant digits, many digits, large
 * and small exponents, the subnormal boundary, hexadecimal.
 */

struct text_case {
	const char *name;
	const char *text;
	int base;
};

static const struct text_case integers[] = {
	{ "strtol/short", "42", 10 },
	{ "strtol/int", "-2147483647", 10 },
	{ "strtol/hex", "0x7fffffff", 0 },
	{ "strtoul/hex16", "deadbeef", 16 },
	{ "strtoll/long", "9223372036854775807", 10 },
	{ "strtoull/hex", "0xfedcba9876543210", 16 },
};

static const struct text_case reals[] = {
	{ "strtod/integer", "12345", 0 },
	{ "strtod/short", "3.14", 0 },
	{ "strtod/fixed", "123456.789", 0 },
	{ "strtod/17-digits", "2.7182818284590452", 0 },
	{ "strtod/long", "3.14159265358979323846264338327950288419716939937510", 0 },
	{ "strtod/exp-small", "1.2345678901234567e-123", 0 },
	{ "strtod/exp-large", "6.02214076e+300", 0 },
	{ "strtod/min-normal", "2.2250738585072014e-308", 0 },
	{ "strtod/subnormal", "4.9406564584124654e-324", 0 },
	{ "strtod/hex", "0x1.921fb54442d18p+1", 0 },
	{ "strtod/micro", "0.000001", 0 },
};

static const struct text_case floats[] = {
	{ "strtof/short", "3.14", 0 },
	{ "strtof/9-digits", "3.14159274", 0 },
	{ "strtof/exp", "1.17549435e-38", 0 },
};

static void run_strtol(void *context, unsigned long iterations)
{
	const struct text_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(c->text);
		bench_sink = (unsigned long)strtol(c->text, nullptr, c->base);
	}
}

static void run_strtoul(void *context, unsigned long iterations)
{
	const struct text_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(c->text);
		bench_sink = strtoul(c->text, nullptr, c->base);
	}
}

static void run_strtoll(void *context, unsigned long iterations)
{
	const struct text_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(c->text);
		bench_sink = (unsigned long)strtoll(c->text, nullptr, c->base);
	}
}

static void run_strtoull(void *context, unsigned long iterations)
{
	const struct text_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(c->text);
		bench_sink = (unsigned long)strtoull(c->text, nullptr, c->base);
	}
}

static void run_atoi(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)atoi("-123456");
}

static void run_strtod(void *context, unsigned long iterations)
{
	const struct text_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(c->text);
		bench_sink = (unsigned long)(long long)(strtod(c->text, nullptr) * 1e3);
	}
}

static void run_strtof(void *context, unsigned long iterations)
{
	const struct text_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(c->text);
		bench_sink = (unsigned long)(long)(strtof(c->text, nullptr) * 1e3f);
	}
}

int main(int argc, char *argv[])
{
	void (*integer_bodies[])(void *context, unsigned long iterations) = { run_strtol, run_strtol, run_strtol, run_strtoul, run_strtoll, run_strtoull };

	bench_init(argc, argv);

	for (size_t i = 0; i < sizeof(integers) / sizeof(integers[0]); ++i)
		bench_run(integers[i].name, 1, integer_bodies[i], (void *)&integers[i]);
	bench_run("atoi", 1, run_atoi, nullptr);
	for (size_t i = 0; i < sizeof(reals) / sizeof(reals[0]); ++i)
		bench_run(reals[i].name, 1, run_strtod, (void *)&reals[i]);
	for (size_t i = 0; i < sizeof(floats) / sizeof(floats[0]); ++i)
		bench_run(floats[i].name, 1, run_strtof, (void *)&floats[i]);

	return bench_finish();
}
