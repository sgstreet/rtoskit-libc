/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdio.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <string.h>

#include "bench.h"

/*
 * Formatted output and input (P3.01, docs/research/benchmarks.md): snprintf
 * per conversion (the integer, string and character ones that dominate real
 * programs, then the floating ones), a typical log line mixing them, the same
 * line through fprintf to a fully buffered stream, and sscanf of the common
 * conversions. Arguments come through volatile objects so nothing is folded.
 */

static char buffer[256];
static FILE *null_stream;

static volatile int int_value = -123456;
static volatile unsigned int hex_value = 0xdeadbeef;
static volatile long long long_value = 1234567890123456789LL;
static const char *volatile string_value = "hello, world";
static volatile double double_value = 3.14159265358979;
static volatile double large_value = 6.02214076e23;
static volatile double small_value = 0.000123456789;

static void run_d(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%d", int_value);
}

static void run_u_small(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%u", (unsigned int)7);
}

static void run_x(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%x", hex_value);
}

static void run_08x(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%08x", hex_value);
}

static void run_lld(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%lld", long_value);
}

static void run_s(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%s", string_value);
}

static void run_s_width(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%-20s|", string_value);
}

static void run_c(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%c", 'x');
}

static void run_p(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%p", (void *)&bench_sink);
}

static void run_literal(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "no conversions at all in this text");
}

static void run_f(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%f", double_value);
}

static void run_3f(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%.3f", large_value / 1e18);
}

static void run_e(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%e", large_value);
}

static void run_g(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%g", small_value);
}

static void run_17g(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%.17g", double_value);
}

static void run_a(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%a", double_value);
}

static void run_mix(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)snprintf(buffer, sizeof(buffer), "%s: id=%d flags=0x%04x len=%u\n", string_value, int_value, hex_value & 0xffff, (unsigned int)i);
}

static void run_fprintf(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)fprintf(null_stream, "%s: id=%d flags=0x%04x len=%u\n", string_value, int_value, hex_value & 0xffff, (unsigned int)i);
}

static void run_fputs(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)fputs("a line of text of a typical length\n", null_stream);
}

static void run_sscanf_d(void *context, unsigned long iterations)
{
	int value;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_sink = (unsigned long)sscanf("-123456", "%d", &value);
		bench_sink += (unsigned long)value;
	}
}

static void run_sscanf_x(void *context, unsigned long iterations)
{
	unsigned int value;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_sink = (unsigned long)sscanf("deadbeef", "%x", &value);
		bench_sink += value;
	}
}

static void run_sscanf_mix(void *context, unsigned long iterations)
{
	char word[32];
	int number;
	unsigned int flags;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_sink = (unsigned long)sscanf("hello: id=-123456 flags=0xbeef", "%31[^:]: id=%d flags=%x", word, &number, &flags);
		bench_sink += (unsigned long)number + flags;
	}
}

static void run_sscanf_f(void *context, unsigned long iterations)
{
	double value;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_sink = (unsigned long)sscanf("3.14159265358979", "%lf", &value);
		bench_sink += (unsigned long)value;
	}
}

int main(int argc, char *argv[])
{
	bench_init(argc, argv);

	bench_run("snprintf/%d", 1, run_d, nullptr);
	bench_run("snprintf/%u-small", 1, run_u_small, nullptr);
	bench_run("snprintf/%x", 1, run_x, nullptr);
	bench_run("snprintf/%08x", 1, run_08x, nullptr);
	bench_run("snprintf/%lld", 1, run_lld, nullptr);
	bench_run("snprintf/%s", 1, run_s, nullptr);
	bench_run("snprintf/%-20s", 1, run_s_width, nullptr);
	bench_run("snprintf/%c", 1, run_c, nullptr);
	bench_run("snprintf/%p", 1, run_p, nullptr);
	bench_run("snprintf/literal", 1, run_literal, nullptr);
	bench_run("snprintf/%f", 1, run_f, nullptr);
	bench_run("snprintf/%.3f", 1, run_3f, nullptr);
	bench_run("snprintf/%e", 1, run_e, nullptr);
	bench_run("snprintf/%g", 1, run_g, nullptr);
	bench_run("snprintf/%.17g", 1, run_17g, nullptr);
	bench_run("snprintf/%a", 1, run_a, nullptr);
	bench_run("snprintf/mix", 1, run_mix, nullptr);

	/* A fully buffered stream whose writes the system discards */
	null_stream = fopen("/dev/null", "w");
	if (null_stream != nullptr) {
		bench_run("fprintf/mix", 1, run_fprintf, nullptr);
		bench_run("fputs/line", 1, run_fputs, nullptr);
	}

	bench_run("sscanf/%d", 1, run_sscanf_d, nullptr);
	bench_run("sscanf/%x", 1, run_sscanf_x, nullptr);
	bench_run("sscanf/mix", 1, run_sscanf_mix, nullptr);
	bench_run("sscanf/%lf", 1, run_sscanf_f, nullptr);

	return bench_finish();
}
