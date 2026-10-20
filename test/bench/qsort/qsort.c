/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * qsort.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <string.h>

#include "bench.h"

/*
 * Sorting and searching (P3.01, docs/research/benchmarks.md): qsort of int
 * arrays (random at three sizes, already sorted, reversed, all equal), of
 * string pointers with strcmp, and bsearch. Each iteration first restores the
 * input with a plain loop (not memcpy, so the library's memcpy does not move
 * these figures); operations are elements sorted or keys searched.
 */

#define LARGEST 1024
#define WORDS 256

struct sort_case {
	const int *input;
	size_t count;
};

static int random_input[LARGEST];
static int sorted_input[LARGEST];
static int reversed_input[LARGEST];
static int equal_input[LARGEST];
static int work[LARGEST];
static char word_storage[WORDS][8];
static const char *word_input[WORDS];
static const char *word_work[WORDS];

static int compare_int(const void *a, const void *b)
{
	int x = *(const int *)a;
	int y = *(const int *)b;

	return (x > y) - (x < y);
}

static int compare_word(const void *a, const void *b)
{
	return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static void run_sort(void *context, unsigned long iterations)
{
	const struct sort_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		for (size_t k = 0; k < c->count; ++k)
			work[k] = c->input[k];
		bench_clobber(work);
		qsort(work, c->count, sizeof(work[0]), compare_int);
	}
}

static void run_sort_words(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		for (size_t k = 0; k < WORDS; ++k)
			word_work[k] = word_input[k];
		bench_clobber(word_work);
		qsort(word_work, WORDS, sizeof(word_work[0]), compare_word);
	}
}

/* 64 keys: every 16th element, each once */
static void run_bsearch(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		for (size_t k = 0; k < LARGEST; k += 16) {
			bench_clobber(sorted_input);
			bench_sink = (unsigned long)bsearch(&sorted_input[k], sorted_input, LARGEST, sizeof(sorted_input[0]), compare_int);
		}
	}
}

static void prepare(void)
{
	unsigned int r = 7;

	for (size_t k = 0; k < LARGEST; ++k) {
		r = r * 1664525U + 1013904223U;
		random_input[k] = (int)(r >> 8);
		sorted_input[k] = (int)k * 3;
		reversed_input[k] = (int)(LARGEST - k);
		equal_input[k] = 42;
	}
	for (size_t k = 0; k < WORDS; ++k) {
		for (size_t j = 0; j < 7; ++j) {
			r = r * 1664525U + 1013904223U;
			word_storage[k][j] = (char)('a' + (r >> 24) % 4);
		}
		word_storage[k][7] = '\0';
		word_input[k] = word_storage[k];
	}
}

int main(int argc, char *argv[])
{
	static const struct sort_case random16 = { random_input, 16 };
	static const struct sort_case random256 = { random_input, 256 };
	static const struct sort_case random1024 = { random_input, LARGEST };
	static const struct sort_case sorted = { sorted_input, LARGEST };
	static const struct sort_case reversed = { reversed_input, LARGEST };
	static const struct sort_case equal = { equal_input, LARGEST };

	bench_init(argc, argv);
	prepare();

	bench_run("qsort/random/16", 16, run_sort, (void *)&random16);
	bench_run("qsort/random/256", 256, run_sort, (void *)&random256);
	bench_run("qsort/random/1024", LARGEST, run_sort, (void *)&random1024);
	bench_run("qsort/sorted/1024", LARGEST, run_sort, (void *)&sorted);
	bench_run("qsort/reversed/1024", LARGEST, run_sort, (void *)&reversed);
	bench_run("qsort/equal/1024", LARGEST, run_sort, (void *)&equal);
	bench_run("qsort/strings/256", WORDS, run_sort_words, nullptr);
	bench_run("bsearch/1024", LARGEST / 16, run_bsearch, nullptr);

	return bench_finish();
}
