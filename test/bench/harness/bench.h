/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bench.h
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _BENCH_H
#define _BENCH_H

/*
 * The benchmark harness (P3.01, docs/research/benchmarks.md). A program
 * registers its cases with bench_run(); each case is a body that performs
 * units operations (calls of the measured function) per iteration and runs
 * "iterations" iterations. Two modes, chosen by argv[1]:
 *
 *   count (default): the body runs once outside the measurement, then once
 *     between bench_roi_begin() and bench_roi_end(), whose addresses the
 *     qemu plugin tools/qemu-plugins/bench-roi.c watches to count the
 *     instructions in between.
 *   time: the iteration count doubles until a run takes BENCH_MIN_CYCLES,
 *     then the fastest of BENCH_REPEATS runs of bench_cycles() is reported
 *     (interference only ever adds time, so on a shared machine the minimum
 *     is the stable statistic; the median moved by tens of percent).
 *
 * Every case prints "bench <name> <operations> <cycles>" on stdout, the
 * operations being iterations x units (cycles is 0 in count mode);
 * tools/scripts/bench-runner pairs the lines with the plugin's and divides.
 */

#include <stddef.h>

#define BENCH_MIN_CYCLES 1000000ULL
#define BENCH_REPEATS 15

/* Somewhere results go so the compiler cannot discard the calls producing them */
extern volatile unsigned long bench_sink;

/* Make the compiler assume that the memory behind pointer is read and changed */
static inline void bench_clobber(const void *pointer)
{
	__asm__ volatile("" : : "r"(pointer) : "memory");
}

void bench_init(int argc, char *argv[]);
void bench_run(const char *name, unsigned long units, void (*body)(void *context, unsigned long iterations), void *context);
int bench_finish(void);

/* The region markers and the cycle counter of the ${ARCH_FAMILY} directory */
void bench_roi_begin(void);
void bench_roi_end(void);
unsigned long long bench_cycles(void);

#endif /* _BENCH_H */
