/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * threads.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <threads.h>

#include "bench.h"

/*
 * <threads.h> without contention (P3.01, docs/research/benchmarks.md): a
 * lock and unlock pair of each mutex type, trylock, call_once once done, and
 * tss_get. Uncontended paths are what a single-core RTOS runs almost always;
 * operations are pairs (or single calls).
 */

static mtx_t plain;
static mtx_t recursive;
static mtx_t timed;
static once_flag once = ONCE_FLAG_INIT;
static tss_t key;
static int value;

static void run_lock(void *context, unsigned long iterations)
{
	mtx_t *mutex = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		mtx_lock(mutex);
		mtx_unlock(mutex);
	}
}

static void run_trylock(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		if (mtx_trylock(&plain) == thrd_success)
			mtx_unlock(&plain);
	}
}

/* The second level of a recursive mutex already held */
static void run_nested(void *context, unsigned long iterations)
{
	mtx_lock(&recursive);
	for (unsigned long i = 0; i < iterations; ++i) {
		mtx_lock(&recursive);
		mtx_unlock(&recursive);
	}
	mtx_unlock(&recursive);
}

static void initialise(void)
{
	++value;
}

static void run_call_once(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		call_once(&once, initialise);
}

static void run_tss_get(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i)
		bench_sink = (unsigned long)tss_get(key);
}

int main(int argc, char *argv[])
{
	bench_init(argc, argv);

	if (mtx_init(&plain, mtx_plain) != thrd_success || mtx_init(&recursive, mtx_plain | mtx_recursive) != thrd_success || mtx_init(&timed, mtx_timed) != thrd_success || tss_create(&key, nullptr) != thrd_success)
		return EXIT_FAILURE;
	tss_set(key, &value);

	bench_run("mtx/plain", 1, run_lock, &plain);
	bench_run("mtx/recursive", 1, run_lock, &recursive);
	bench_run("mtx/timed", 1, run_lock, &timed);
	bench_run("mtx/trylock", 1, run_trylock, nullptr);
	bench_run("mtx/nested", 1, run_nested, nullptr);
	bench_run("call_once/done", 1, run_call_once, nullptr);
	bench_run("tss_get", 1, run_tss_get, nullptr);

	return bench_finish();
}
