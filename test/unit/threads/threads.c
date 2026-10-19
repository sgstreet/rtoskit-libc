/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * threads.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <rtos/rtos-port.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#include "internal/malloc.h"

/*
 * <threads.h> and the thread-safety of the library (P1.12,
 * docs/research/threads.md): thread creation and results, per-thread
 * library state, mutexes, condition variables, call_once, thread-specific
 * storage, and malloc and stdio under contention.
 */

#define THREADS 4

static int failures;
static mtx_t report_lock;

static void fail(int line, const char *what)
{
	mtx_lock(&report_lock);
	printf("fail: line %d: %s\n", line, what);
	++failures;
	mtx_unlock(&report_lock);
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

/* now + milliseconds on TIME_UTC */
static struct timespec after(long milliseconds)
{
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);
	ts.tv_nsec += (milliseconds % 1000) * 1000000;
	ts.tv_sec += milliseconds / 1000 + ts.tv_nsec / 1000000000;
	ts.tv_nsec %= 1000000000;

	return ts;
}

static long long elapsed_ms(const struct timespec *start)
{
	struct timespec now;

	timespec_get(&now, TIME_UTC);
	return (now.tv_sec - start->tv_sec) * 1000LL + (now.tv_nsec - start->tv_nsec) / 1000000;
}

/* Creation, results, identity */

static thread_local int tls_counter = 7;
/*
 * Zero-initialised and more aligned than the initialised data, so the TLS
 * segment's address need not be a multiple of its alignment
 * (p_vaddr % p_align != 0, a P1.01 follow-up): __libc_tls_init must align
 * the block, not the image.
 */
alignas(64) static thread_local char tls_aligned[64];

static int identity(void *arg)
{
	thrd_t *self = arg;

	*self = thrd_current();
	/* Fresh TLS: the initialised values, not the creator's */
	if (tls_counter != 7 || tls_aligned[0] != 0 || tls_aligned[63] != 0 || ((uintptr_t)tls_aligned & 63) != 0)
		return -1;
	strcpy(tls_aligned, "aligned");
	tls_counter = 100;
	return 42;
}

static void check_create(void)
{
	thrd_t thread;
	thrd_t seen = nullptr;
	int result = 0;

	tls_counter = 1;
	strcpy(tls_aligned, "initial thread");
	CHECK(((uintptr_t)tls_aligned & 63) == 0);
	CHECK(thrd_create(&thread, identity, &seen) == thrd_success);
	CHECK(thrd_join(thread, &result) == thrd_success && result == 42);
	CHECK(thrd_equal(seen, thread) && !thrd_equal(seen, thrd_current()));
	CHECK(thrd_equal(thrd_current(), thrd_current()));
	CHECK(tls_counter == 1 && strcmp(tls_aligned, "initial thread") == 0);
	/* A null result pointer, and joining twice is an error the port reports */
	CHECK(thrd_create(&thread, identity, &seen) == thrd_success && thrd_join(thread, nullptr) == thrd_success);
	CHECK(thrd_join(thrd_current(), nullptr) == thrd_error);
}

/*
 * A TLS image whose address is not a multiple of its alignment (GNU ld
 * always aligns the segment, so only a hand-made image shows it): the data
 * must land at the same phase, initialised then zero filled, inside the
 * block, whatever the block's own alignment.
 */
static void check_tls_phase(void)
{
	alignas(64) static unsigned char segment[128];
	static const unsigned char pattern[8] = { 0xa5, 'T', 'L', 'S', 'I', 'M', 'G', 0x5a };
	struct __libc_tls_image image = { .init = &segment[4], .init_size = sizeof(pattern), .size = 60, .align = 64 };

	memcpy(&segment[4], pattern, sizeof(pattern));
	for (size_t offset = 0; offset < 64; offset += 12) {
		size_t size = __libc_tls_block_size(&image);
		unsigned char *storage = malloc(size + offset);
		unsigned char *block = storage + offset;
		unsigned char *data = nullptr;

		CHECK(storage != nullptr);
		if (storage == nullptr)
			return;
		memset(block, 0xee, size);
		CHECK(__libc_tls_init(block, &image) != nullptr);
		for (size_t i = 0; i + sizeof(pattern) <= size; ++i) {
			if (memcmp(block + i, pattern, sizeof(pattern)) == 0) {
				data = block + i;
				break;
			}
		}
		CHECK(data != nullptr && ((uintptr_t)data & 63) == 4 && data + image.size <= block + size);
		for (size_t i = sizeof(pattern); data != nullptr && i < image.size; ++i)
			CHECK(data[i] == 0);
		free(storage);
	}
}

/* Per-thread library state */

static int library_state(void *arg)
{
	char text[] = "a,b,c";
	unsigned int seed = (unsigned int)(uintptr_t)arg;
	int sequence[4];
	int status = 0;

	errno = 0;
	strtol("99999999999999999999999", nullptr, 10);
	if (errno != ERANGE)
		status = 1;
	srand(seed);
	for (int i = 0; i < 4; ++i)
		sequence[i] = rand();
	if (strcmp(strtok(text, ","), "a") != 0)
		status = 2;
	thrd_yield();
	if (strcmp(strtok(nullptr, ","), "b") != 0)
		status = 3;
	srand(seed);
	for (int i = 0; i < 4; ++i)
		if (rand() != sequence[i])
			status = 4;

	return status;
}

static void check_library_state(void)
{
	char text[] = "x;y";
	thrd_t threads[THREADS];
	int result;

	errno = EDOM;
	CHECK(strcmp(strtok(text, ";"), "x") == 0);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], library_state, (void *)(uintptr_t)(i + 1)) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], &result) == thrd_success && result == 0);
	/* This thread's errno and strtok position are untouched */
	CHECK(errno == EDOM);
	CHECK(strcmp(strtok(nullptr, ";"), "y") == 0);
}

/* Mutexes */

#define INCREMENTS 2000

static mtx_t counter_lock;
static long counter;

static int increment(void *arg)
{
	int recursive = *(int *)arg;

	for (int i = 0; i < INCREMENTS; ++i) {
		if (mtx_lock(&counter_lock) != thrd_success)
			return 1;
		if (recursive && mtx_lock(&counter_lock) != thrd_success)
			return 2;
		long value = counter;

		if (i % 64 == 0)
			thrd_yield();
		counter = value + 1;
		if (recursive && mtx_unlock(&counter_lock) != thrd_success)
			return 3;
		if (mtx_unlock(&counter_lock) != thrd_success)
			return 4;
	}

	return 0;
}

static int try_from_other(void *arg)
{
	mtx_t *mtx = arg;
	struct timespec deadline = after(50);
	struct timespec start;
	int status;

	if (mtx_trylock(mtx) != thrd_busy)
		return 1;
	timespec_get(&start, TIME_UTC);
	status = mtx_timedlock(mtx, &deadline);
	if (status != thrd_timedout || elapsed_ms(&start) < 40)
		return 2;
	return 0;
}

static void check_mutex(void)
{
	thrd_t threads[THREADS];
	int recursive[2] = { 0, 1 };
	int result;
	mtx_t mtx;

	for (int r = 0; r < 2; ++r) {
		counter = 0;
		CHECK(mtx_init(&counter_lock, r ? mtx_plain | mtx_recursive : mtx_plain) == thrd_success);
		for (int i = 0; i < THREADS; ++i)
			CHECK(thrd_create(&threads[i], increment, &recursive[r]) == thrd_success);
		for (int i = 0; i < THREADS; ++i)
			CHECK(thrd_join(threads[i], &result) == thrd_success && result == 0);
		CHECK(counter == THREADS * INCREMENTS);
		mtx_destroy(&counter_lock);
	}

	/* Recursion depth, and unlocking a mutex that is not held */
	CHECK(mtx_init(&mtx, mtx_timed | mtx_recursive) == thrd_success);
	for (int i = 0; i < 5; ++i)
		CHECK(mtx_lock(&mtx) == thrd_success);
	CHECK(mtx_trylock(&mtx) == thrd_success);
	for (int i = 0; i < 6; ++i)
		CHECK(mtx_unlock(&mtx) == thrd_success);
	CHECK(mtx_unlock(&mtx) == thrd_error);

	/* Busy and timed out from another thread while this one holds it */
	CHECK(mtx_lock(&mtx) == thrd_success);
	CHECK(thrd_create(&threads[0], try_from_other, &mtx) == thrd_success);
	CHECK(thrd_join(threads[0], &result) == thrd_success && result == 0);
	CHECK(mtx_unlock(&mtx) == thrd_success);
	mtx_destroy(&mtx);

	/* A plain mutex refuses a second lock by trylock; timed locks of untimed mutexes and unknown types fail (mtx_plain is 0, so mtx_recursive alone is mtx_plain | mtx_recursive) */
	CHECK(mtx_init(&mtx, mtx_plain) == thrd_success && mtx_trylock(&mtx) == thrd_success && mtx_trylock(&mtx) == thrd_busy && mtx_unlock(&mtx) == thrd_success);
	struct timespec soon = after(10);

	CHECK(mtx_timedlock(&mtx, &soon) == thrd_error);
	/* A timed mutex the caller holds is not recursive: a timed lock times out */
	CHECK(mtx_init(&mtx, mtx_timed) == thrd_success && mtx_lock(&mtx) == thrd_success);
	soon = after(20);
	CHECK(mtx_timedlock(&mtx, &soon) == thrd_timedout && mtx_unlock(&mtx) == thrd_success);
	CHECK(mtx_init(&mtx, 4) == thrd_error && mtx_init(&mtx, -1) == thrd_error);
	/* An uncontended timed lock succeeds even with a deadline in the past */
	CHECK(mtx_init(&mtx, mtx_timed) == thrd_success);
	soon.tv_sec -= 10;
	CHECK(mtx_timedlock(&mtx, &soon) == thrd_success && mtx_unlock(&mtx) == thrd_success);
}

/* Condition variables: a bounded queue */

#define ITEMS 500
#define QUEUE 8

static mtx_t queue_lock;
static cnd_t not_empty;
static cnd_t not_full;
static int queue[QUEUE];
static int queue_count;
static int queue_head;

static int producer(void *)
{
	for (int i = 1; i <= ITEMS; ++i) {
		mtx_lock(&queue_lock);
		while (queue_count == QUEUE)
			cnd_wait(&not_full, &queue_lock);
		queue[(queue_head + queue_count++) % QUEUE] = i;
		cnd_signal(&not_empty);
		mtx_unlock(&queue_lock);
	}

	return 0;
}

static int consumer(void *arg)
{
	long *sum = arg;

	while (true) {
		int item;

		mtx_lock(&queue_lock);
		while (queue_count == 0)
			cnd_wait(&not_empty, &queue_lock);
		item = queue[queue_head];
		queue_head = (queue_head + 1) % QUEUE;
		--queue_count;
		cnd_signal(&not_full);
		mtx_unlock(&queue_lock);
		if (item < 0)
			return 0;
		*sum += item;
	}
}

static int gate_open;
static int gate_waiting;

static int wait_gate(void *)
{
	mtx_lock(&queue_lock);
	++gate_waiting;
	cnd_signal(&not_full);
	while (!gate_open)
		cnd_wait(&not_empty, &queue_lock);
	mtx_unlock(&queue_lock);

	return 0;
}

static void check_condition(void)
{
	thrd_t producers[2];
	thrd_t consumers[2];
	long sums[2] = { 0, 0 };
	struct timespec start;
	struct timespec deadline;
	int result;

	CHECK(mtx_init(&queue_lock, mtx_plain) == thrd_success && cnd_init(&not_empty) == thrd_success && cnd_init(&not_full) == thrd_success);
	for (int i = 0; i < 2; ++i) {
		CHECK(thrd_create(&consumers[i], consumer, &sums[i]) == thrd_success);
		CHECK(thrd_create(&producers[i], producer, nullptr) == thrd_success);
	}
	for (int i = 0; i < 2; ++i)
		CHECK(thrd_join(producers[i], &result) == thrd_success && result == 0);
	/* One stop marker per consumer */
	for (int i = 0; i < 2; ++i) {
		mtx_lock(&queue_lock);
		while (queue_count == QUEUE)
			cnd_wait(&not_full, &queue_lock);
		queue[(queue_head + queue_count++) % QUEUE] = -1;
		cnd_signal(&not_empty);
		mtx_unlock(&queue_lock);
	}
	for (int i = 0; i < 2; ++i)
		CHECK(thrd_join(consumers[i], &result) == thrd_success && result == 0);
	CHECK(sums[0] + sums[1] == 2L * ITEMS * (ITEMS + 1) / 2);

	/* Broadcast releases every waiter */
	thrd_t waiters[THREADS];

	gate_open = 0;
	gate_waiting = 0;
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&waiters[i], wait_gate, nullptr) == thrd_success);
	mtx_lock(&queue_lock);
	while (gate_waiting < THREADS)
		cnd_wait(&not_full, &queue_lock);
	gate_open = 1;
	CHECK(cnd_broadcast(&not_empty) == thrd_success);
	mtx_unlock(&queue_lock);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(waiters[i], &result) == thrd_success);

	/* A timed wait times out with the mutex held again */
	mtx_lock(&queue_lock);
	timespec_get(&start, TIME_UTC);
	deadline = after(50);
	CHECK(cnd_timedwait(&not_empty, &queue_lock, &deadline) == thrd_timedout && elapsed_ms(&start) >= 40);
	CHECK(mtx_unlock(&queue_lock) == thrd_success && mtx_unlock(&queue_lock) == thrd_error);
	/* Signals with no waiter do nothing */
	CHECK(cnd_signal(&not_empty) == thrd_success && cnd_broadcast(&not_empty) == thrd_success);
	cnd_destroy(&not_empty);
	cnd_destroy(&not_full);
	mtx_destroy(&queue_lock);
}

/*
 * Ping-pong: each thread waits for its turn and hands it over with a signal.
 * A signal that arrives after a waiter released the mutex but before it
 * slept must not be lost, or both threads wait forever.
 */
#define PING_PONG 5000

static mtx_t turn_lock;
static cnd_t turn_changed;
static int turn;

static int ping_pong(void *arg)
{
	int me = *(int *)arg;

	for (int i = 0; i < PING_PONG; ++i) {
		mtx_lock(&turn_lock);
		while (turn != me)
			cnd_wait(&turn_changed, &turn_lock);
		turn = 1 - me;
		cnd_signal(&turn_changed);
		mtx_unlock(&turn_lock);
	}

	return 0;
}

static void check_ping_pong(void)
{
	thrd_t threads[2];
	int ids[2] = { 0, 1 };

	CHECK(mtx_init(&turn_lock, mtx_plain) == thrd_success && cnd_init(&turn_changed) == thrd_success);
	for (int i = 0; i < 2; ++i)
		CHECK(thrd_create(&threads[i], ping_pong, &ids[i]) == thrd_success);
	for (int i = 0; i < 2; ++i)
		CHECK(thrd_join(threads[i], nullptr) == thrd_success);
	CHECK(turn == 0);
}

/* call_once */

static once_flag once = ONCE_FLAG_INIT;
static int once_calls;

static void once_function(void)
{
	thrd_sleep(&(struct timespec){ .tv_nsec = 20000000 }, nullptr);
	++once_calls;
}

static int call_once_thread(void *)
{
	call_once(&once, once_function);
	return once_calls;
}

static void check_call_once(void)
{
	thrd_t threads[THREADS];
	int result;

	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], call_once_thread, nullptr) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], &result) == thrd_success && result == 1);
	call_once(&once, once_function);
	CHECK(once_calls == 1);
}

/* Thread-specific storage */

static tss_t key_a;
static tss_t key_b;
static int destructed[2];
static int reset_count;

static void destroy_a(void *value)
{
	__atomic_fetch_add(&destructed[0], *(int *)value, __ATOMIC_RELAXED);
}

/* Sets its value again twice; the passes stop at TSS_DTOR_ITERATIONS */
static void destroy_b(void *value)
{
	__atomic_fetch_add(&destructed[1], 1, __ATOMIC_RELAXED);
	if (__atomic_add_fetch(&reset_count, 1, __ATOMIC_RELAXED) < 3)
		tss_set(key_b, value);
}

static int use_tss(void *arg)
{
	static int weights[THREADS] = { 1, 10, 100, 1000 };
	int index = *(int *)arg;

	if (tss_get(key_a) != nullptr)
		return 1;
	if (tss_set(key_a, &weights[index]) != thrd_success || tss_get(key_a) != &weights[index])
		return 2;
	thrd_yield();
	if (tss_get(key_a) != &weights[index])
		return 3;

	return 0;
}

static int forever_b(void *)
{
	static int token;

	tss_set(key_b, &token);
	return 0;
}

static void check_tss(void)
{
	thrd_t threads[THREADS];
	int indices[THREADS] = { 0, 1, 2, 3 };
	int local = 5;
	int result;
	tss_t keys[40];
	int created = 0;

	CHECK(tss_create(&key_a, destroy_a) == thrd_success && tss_create(&key_b, destroy_b) == thrd_success && key_a != key_b);
	CHECK(tss_set(key_a, &local) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], use_tss, &indices[i]) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], &result) == thrd_success && result == 0);
	CHECK(destructed[0] == 1111);
	CHECK(tss_get(key_a) == &local);

	/* A destructor that keeps setting a value runs again, a bounded number of times */
	CHECK(thrd_create(&threads[0], forever_b, nullptr) == thrd_success && thrd_join(threads[0], &result) == thrd_success);
	CHECK(destructed[1] == 3 && destructed[1] <= TSS_DTOR_ITERATIONS);

	/* A deleted key's values are gone; a reused key starts null in every thread */
	tss_delete(key_a);
	CHECK(tss_get(key_a) == nullptr && tss_set(key_a, &local) == thrd_error);
	CHECK(tss_create(&keys[0], nullptr) == thrd_success && tss_get(keys[0]) == nullptr);
	tss_delete(keys[0]);
	tss_delete(key_b);

	/* The table is finite; every created key can be deleted and reused */
	while (created < 40 && tss_create(&keys[created], nullptr) == thrd_success)
		++created;
	CHECK(created >= 16 && created < 40);
	for (int i = 0; i < created; ++i)
		tss_delete(keys[i]);
	CHECK(tss_create(&keys[0], nullptr) == thrd_success);
	tss_delete(keys[0]);
}

/* Detached threads: before and after they end, and many of them */

static int detached_done;

static int detached(void *)
{
	__atomic_fetch_add(&detached_done, 1, __ATOMIC_RELEASE);
	return 0;
}

static void check_detach(void)
{
	thrd_t thread;

	for (int i = 0; i < 64; ++i) {
		CHECK(thrd_create(&thread, detached, nullptr) == thrd_success);
		if (i % 2 == 0)
			thrd_yield();
		CHECK(thrd_detach(thread) == thrd_success);
	}
	while (__atomic_load_n(&detached_done, __ATOMIC_ACQUIRE) < 64)
		thrd_yield();
	CHECK(detached_done == 64);
}

/* Sleeping */

static void check_sleep(void)
{
	struct timespec start;
	struct timespec duration = { .tv_sec = 0, .tv_nsec = 30000000 };

	timespec_get(&start, TIME_UTC);
	CHECK(thrd_sleep(&duration, &duration) == 0 && elapsed_ms(&start) >= 30);
	CHECK(thrd_sleep(&(struct timespec){ .tv_sec = 0, .tv_nsec = 1000000000 }, nullptr) < -1);
	CHECK(thrd_sleep(&(struct timespec){ .tv_sec = -1, .tv_nsec = 0 }, nullptr) < -1);
}

/* malloc and stdio from several threads */

static int allocate(void *arg)
{
	unsigned int seed = (unsigned int)(uintptr_t)arg;
	unsigned char *blocks[16] = { nullptr };
	size_t sizes[16] = { 0 };

	for (int round = 0; round < 3000; ++round) {
		int slot;
		unsigned char fill = (unsigned char)(seed + round);

		seed = seed * 1103515245 + 12345;
		slot = (seed >> 16) % 16;
		if (blocks[slot] != nullptr) {
			for (size_t i = 0; i < sizes[slot]; ++i)
				if (blocks[slot][i] != blocks[slot][0])
					return 1;
			free(blocks[slot]);
		}
		sizes[slot] = 1 + (seed >> 8) % 300;
		blocks[slot] = malloc(sizes[slot]);
		if (blocks[slot] == nullptr)
			return 2;
		memset(blocks[slot], fill, sizes[slot]);
	}
	for (int slot = 0; slot < 16; ++slot)
		free(blocks[slot]);

	return 0;
}

static FILE *shared_file;

static int write_lines(void *arg)
{
	int id = *(int *)arg;

	for (int i = 0; i < 200; ++i)
		if (fprintf(shared_file, "thread %d line %03d %s\n", id, i, "................................................") < 0)
			return 1;

	return 0;
}

static void check_contention(void)
{
	thrd_t threads[THREADS];
	int ids[THREADS] = { 0, 1, 2, 3 };
	int lines[THREADS] = { 0 };
	char name[L_tmpnam];
	char line[128];
	int result;

	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], allocate, (void *)(uintptr_t)(i * 7919 + 1)) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], &result) == thrd_success && result == 0);
	CHECK(__libc_malloc_check());

	/* Each fprintf holds the stream's lock, so lines never mix */
	CHECK(tmpnam(name) != nullptr && (shared_file = fopen(name, "w+")) != nullptr);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], write_lines, &ids[i]) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], &result) == thrd_success && result == 0);
	rewind(shared_file);
	while (fgets(line, sizeof(line), shared_file) != nullptr) {
		int id;
		int number;
		char dots[64];

		if (sscanf(line, "thread %d line %d %63s", &id, &number, dots) != 3 || id < 0 || id >= THREADS || number != lines[id] || strlen(dots) != 48) {
			fail(__LINE__, line);
			break;
		}
		++lines[id];
	}
	for (int i = 0; i < THREADS; ++i)
		CHECK(lines[i] == 200);
	CHECK(fclose(shared_file) == 0 && remove(name) == 0);
}

int main(void)
{
	mtx_init(&report_lock, mtx_plain);

	check_create();
	check_tls_phase();
	check_library_state();
	check_mutex();
	check_condition();
	check_ping_pong();
	check_call_once();
	check_tss();
	check_detach();
	check_sleep();
	check_contention();

	if (failures == 0)
		puts("pass: threads");

	return failures == 0 ? 0 : 1;
}
