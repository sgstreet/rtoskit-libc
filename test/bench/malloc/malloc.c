/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * malloc.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <string.h>

#include "bench.h"

/*
 * Allocator patterns (P3.01, docs/research/benchmarks.md): a block of one
 * size allocated and freed again (the steady state of most embedded code),
 * batches of mixed sizes freed in allocation order, in reverse and in a
 * shuffled order, a buffer grown by realloc, and calloc. Operations are the
 * malloc, realloc, calloc and free calls together. bench-malloc links libc.a
 * (the LIBC_MALLOC allocator), bench-malloc-tiny libc-tinymalloc.a (D54).
 */

#define BATCH 64

static void *blocks[BATCH];
static size_t batch_sizes[BATCH];
static size_t shuffled[BATCH];

static void run_fixed(void *context, unsigned long iterations)
{
	size_t size = (size_t)(unsigned long)context;

	for (unsigned long i = 0; i < iterations; ++i) {
		void *block = malloc(size);

		bench_clobber(block);
		free(block);
	}
}

static void allocate_batch(void)
{
	for (size_t k = 0; k < BATCH; ++k) {
		blocks[k] = malloc(batch_sizes[k]);
		bench_clobber(blocks[k]);
	}
}

static void run_fifo(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		allocate_batch();
		for (size_t k = 0; k < BATCH; ++k)
			free(blocks[k]);
	}
}

static void run_lifo(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		allocate_batch();
		for (size_t k = BATCH; k > 0; --k)
			free(blocks[k - 1]);
	}
}

static void run_shuffled(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		allocate_batch();
		for (size_t k = 0; k < BATCH; ++k)
			free(blocks[shuffled[k]]);
	}
}

/* Half the batch freed and allocated again in a different size: the free list holds holes */
static void run_churn(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		allocate_batch();
		for (size_t k = 0; k < BATCH; k += 2) {
			free(blocks[shuffled[k]]);
			blocks[shuffled[k]] = malloc(batch_sizes[k + 1]);
			bench_clobber(blocks[shuffled[k]]);
		}
		for (size_t k = 0; k < BATCH; ++k)
			free(blocks[k]);
	}
}

/* 16 bytes doubled to 4096 by realloc: 9 reallocs, a malloc and a free */
static void run_realloc(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		char *buffer = malloc(16);

		for (size_t size = 32; size <= 4096; size *= 2) {
			buffer = realloc(buffer, size);
			bench_clobber(buffer);
		}
		free(buffer);
	}
}

static void run_calloc(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		void *block = calloc(32, 8);

		bench_clobber(block);
		free(block);
	}
}

/* Sizes from 8 to 263 bytes and a fixed shuffle (a linear congruential generator) */
static void prepare(void)
{
	unsigned int r = 2026;

	for (size_t k = 0; k < BATCH; ++k) {
		r = r * 1664525U + 1013904223U;
		batch_sizes[k] = 8 + (r >> 24);
		shuffled[k] = k;
	}
	for (size_t k = BATCH - 1; k > 0; --k) {
		size_t j;
		size_t t;

		r = r * 1664525U + 1013904223U;
		j = (r >> 16) % (k + 1);
		t = shuffled[k];
		shuffled[k] = shuffled[j];
		shuffled[j] = t;
	}
}

int main(int argc, char *argv[])
{
	bench_init(argc, argv);
	prepare();

	bench_run("fixed/16", 2, run_fixed, (void *)16UL);
	bench_run("fixed/256", 2, run_fixed, (void *)256UL);
	bench_run("fixed/4096", 2, run_fixed, (void *)4096UL);
	bench_run("batch/fifo", 2 * BATCH, run_fifo, nullptr);
	bench_run("batch/lifo", 2 * BATCH, run_lifo, nullptr);
	bench_run("batch/shuffled", 2 * BATCH, run_shuffled, nullptr);
	bench_run("batch/churn", 3 * BATCH, run_churn, nullptr);
	bench_run("realloc/grow", 11, run_realloc, nullptr);
	bench_run("calloc/256", 2, run_calloc, nullptr);

	return bench_finish();
}
