/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bench.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

volatile unsigned long bench_sink;

static bool timing;
static unsigned long cases;

/*
 * The markers do nothing; the plugin acts on reaching their first
 * instruction. noipa keeps every call real and out of line, so the
 * instructions they add are the same for every region and the empty region
 * that bench_init() measures first calibrates them away.
 */
[[gnu::noipa]] void bench_roi_begin(void)
{
	__asm__ volatile("" ::: "memory");
}

[[gnu::noipa]] void bench_roi_end(void)
{
	__asm__ volatile("" ::: "memory");
}

static void empty(void *context, unsigned long iterations)
{
}

static int compare_cycles(const void *a, const void *b)
{
	unsigned long long x = *(const unsigned long long *)a;
	unsigned long long y = *(const unsigned long long *)b;

	return (x > y) - (x < y);
}

static unsigned long long time_once(void (*body)(void *context, unsigned long iterations), void *context, unsigned long iterations)
{
	unsigned long long start = bench_cycles();

	body(context, iterations);
	return bench_cycles() - start;
}

static void run_timed(const char *name, unsigned long units, void (*body)(void *context, unsigned long iterations), void *context)
{
	unsigned long long samples[BENCH_REPEATS];
	unsigned long iterations = 1;

	/* Warm up, then find an iteration count long enough to time */
	body(context, 1);
	while (time_once(body, context, iterations) < BENCH_MIN_CYCLES && iterations < (1UL << 30))
		iterations *= 2;

	for (size_t i = 0; i < BENCH_REPEATS; ++i)
		samples[i] = time_once(body, context, iterations);
	qsort(samples, BENCH_REPEATS, sizeof(samples[0]), compare_cycles);
	printf("bench %s %lu %llu\n", name, iterations * units, samples[0]);
}

static void run_counted(const char *name, unsigned long units, void (*body)(void *context, unsigned long iterations), void *context)
{
	/* The first run takes one-time costs (heap growth, stream buffers) out of the count */
	body(context, 1);
	bench_roi_begin();
	body(context, 1);
	bench_roi_end();
	printf("bench %s %lu 0\n", name, units);
}

void bench_run(const char *name, unsigned long units, void (*body)(void *context, unsigned long iterations), void *context)
{
	++cases;
	if (timing)
		run_timed(name, units, body, context);
	else
		run_counted(name, units, body, context);
}

void bench_init(int argc, char *argv[])
{
	timing = argc > 1 && strcmp(argv[1], "time") == 0;

	/* The calibration region: what the markers and the call of a body cost */
	bench_run("@overhead", 1, empty, nullptr);
}

int bench_finish(void)
{
	return fflush(stdout) == 0 && cases > 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
