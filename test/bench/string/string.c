/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * string.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <string.h>

#include "bench.h"

/*
 * Memory and string functions over a size x alignment sweep (P3.01,
 * docs/research/benchmarks.md): every size in sizes[] at each offset pair in
 * offsets[] (source:destination modulo 4, or the one pointer of memset,
 * memchr, strlen, strchr), plus a mixed case of 256 calls with small sizes
 * from a fixed generator, the shape of real traffic. The searches find what
 * they look for at the last byte; the comparisons compare equal buffers, so
 * each call reads the whole size.
 */

#define BUFFER 2048
#define BASE 64
#define MIX_CALLS 256

struct string_case {
	size_t size;
	size_t source;
	size_t target;
};

static const size_t sizes[] = { 0, 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 64, 128, 256, 1024 };
static const size_t offsets[][2] = { { 0, 0 }, { 1, 1 }, { 0, 1 }, { 3, 2 } };
static const size_t single_offsets[] = { 0, 1, 2, 3 };

static unsigned char source[BUFFER] __attribute__((aligned(8)));
static unsigned char target[BUFFER] __attribute__((aligned(8)));
static size_t mix_sizes[MIX_CALLS];

static void run_memcpy(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		memcpy(target + c->target, source + c->source, c->size);
		bench_clobber(target);
	}
}

/* Overlapping moves within one buffer: the destination 8 bytes below or above the source */
static void run_memmove_down(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		memmove(target + c->target - 8, target + c->source, c->size);
		bench_clobber(target);
	}
}

static void run_memmove_up(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		memmove(target + c->target + 8, target + c->source, c->size);
		bench_clobber(target);
	}
}

static void run_memset(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		memset(target + c->target, (int)i, c->size);
		bench_clobber(target);
	}
}

static void run_memcmp(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(target);
		bench_sink = (unsigned long)memcmp(target + c->target, source + c->source, c->size);
	}
}

static void run_memchr(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(source);
		bench_sink = (unsigned long)memchr(source + c->source, 'z', c->size);
	}
}

static void run_strlen(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(source);
		bench_sink = strlen((const char *)source + c->source);
	}
}

static void run_strchr(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(source);
		bench_sink = (unsigned long)strchr((const char *)source + c->source, 'z');
	}
}

static void run_strcmp(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		bench_clobber(target);
		bench_sink = (unsigned long)strcmp((const char *)target + c->target, (const char *)source + c->source);
	}
}

static void run_strcpy(void *context, unsigned long iterations)
{
	const struct string_case *c = context;

	for (unsigned long i = 0; i < iterations; ++i) {
		strcpy((char *)target + c->target, (const char *)source + c->source);
		bench_clobber(target);
	}
}

static void run_mix_memcpy(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		for (size_t k = 0; k < MIX_CALLS; ++k)
			memcpy(target + (k & 7), source + (k >> 3 & 7), mix_sizes[k]);
		bench_clobber(target);
	}
}

static void run_mix_memset(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		for (size_t k = 0; k < MIX_CALLS; ++k)
			memset(target + (k & 7), 0, mix_sizes[k]);
		bench_clobber(target);
	}
}

static void run_mix_strlen(void *context, unsigned long iterations)
{
	for (unsigned long i = 0; i < iterations; ++i) {
		unsigned long sum = 0;

		bench_clobber(source);
		for (size_t k = 0; k < MIX_CALLS; ++k)
			sum += strlen((const char *)source + BUFFER / 2 + (k & 7) - mix_sizes[k]);
		bench_sink = sum;
	}
}

/*
 * Both buffers hold the same string of size bytes at the case's offsets,
 * 'z' as its last byte (what memchr and strchr find); the strings are
 * terminated and everything else is a non-zero filler.
 */
static void prepare(const struct string_case *c)
{
	memset(source, 'a', sizeof(source));
	memset(target, 'a', sizeof(target));
	if (c->size > 0) {
		source[c->source + c->size - 1] = 'z';
		target[c->target + c->size - 1] = 'z';
	}
	source[c->source + c->size] = '\0';
	target[c->target + c->size] = '\0';
}

static void sweep(const char *function, void (*body)(void *context, unsigned long iterations), bool two_pointers)
{
	char name[64];

	for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s) {
		for (size_t o = 0; o < sizeof(offsets) / sizeof(offsets[0]); ++o) {
			size_t first = two_pointers ? offsets[o][0] : single_offsets[o];
			struct string_case c = { sizes[s], BASE + first, BASE + (two_pointers ? offsets[o][1] : first) };

			if (two_pointers)
				snprintf(name, sizeof(name), "%s/%zu/%zu:%zu", function, sizes[s], offsets[o][0], offsets[o][1]);
			else
				snprintf(name, sizeof(name), "%s/%zu/%zu", function, sizes[s], first);
			prepare(&c);
			bench_run(name, 1, body, &c);
		}
	}
}

/*
 * Small sizes, most below 16 and a tail to 255: a 32-bit linear congruential
 * generator, size = (r >> 24) >> ((r >> 8) % 5). Fixed, so every run and
 * every library sees the same calls.
 */
static void prepare_mix(void)
{
	unsigned int r = 12345;

	for (size_t k = 0; k < MIX_CALLS; ++k) {
		r = r * 1664525U + 1013904223U;
		mix_sizes[k] = (r >> 24) >> ((r >> 8) % 5);
	}
	memset(source, 'a', sizeof(source));
	source[BUFFER / 2 + 8] = '\0';
}

int main(int argc, char *argv[])
{
	bench_init(argc, argv);

	sweep("memcpy", run_memcpy, true);
	sweep("memmove-down", run_memmove_down, true);
	sweep("memmove-up", run_memmove_up, true);
	sweep("memset", run_memset, false);
	sweep("memcmp", run_memcmp, true);
	sweep("memchr", run_memchr, false);
	sweep("strlen", run_strlen, false);
	sweep("strchr", run_strchr, false);
	sweep("strcmp", run_strcmp, true);
	sweep("strcpy", run_strcpy, true);

	prepare_mix();
	bench_run("mix/memcpy", MIX_CALLS, run_mix_memcpy, nullptr);
	bench_run("mix/memset", MIX_CALLS, run_mix_memset, nullptr);
	bench_run("mix/strlen", MIX_CALLS, run_mix_strlen, nullptr);

	return bench_finish();
}
