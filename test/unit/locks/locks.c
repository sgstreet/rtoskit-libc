/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * locks.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos-port.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>

/*
 * The seam's lock word (P3.04a, PLAN D79): its states, nested acquisition
 * through the port's functions and through the in-line fast path the
 * contract allows libc (below) in any mix, distinct tokens per thread, and
 * mutual exclusion under contention with nesting. The library's own lock
 * users (malloc, stdio) run under contention in test/unit/threads.
 */

#define THREADS 4
#define ROUNDS 3000

#define CHECK(condition) check(__LINE__, (condition), #condition)

static int failures;

static void check(int line, bool ok, const char *what)
{
	if (!ok) {
		printf("FAIL line %d: %s\n", line, what);
		++failures;
	}
}

/* The fast path rtos-port.h allows: the lock word is the first member of struct __rtos_lock */
static void fast_acquire(struct __rtos_lock *lock)
{
	unsigned int word = 0;

	if (!__atomic_compare_exchange_n((unsigned int *)lock, &word, __rtos_lock_token, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		__rtos_lock_acquire(lock);
}

static void fast_release(struct __rtos_lock *lock)
{
	unsigned int word = __rtos_lock_token;

	if (!__atomic_compare_exchange_n((unsigned int *)lock, &word, 0, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
		__rtos_lock_release(lock);
}

static unsigned int word(struct __rtos_lock *lock)
{
	return __atomic_load_n((unsigned int *)lock, __ATOMIC_RELAXED);
}

/* One thread: what the word holds after each step */
static void check_states(void)
{
	struct __rtos_lock *lock;
	unsigned int self = __rtos_lock_token;

	CHECK(self != 0 && (self & __RTOS_LOCK_SLOW) == 0);
	CHECK(__rtos_lock_create(&lock) == 0);
	CHECK(word(lock) == 0);

	/* Held once, by either path, the word is the token alone */
	fast_acquire(lock);
	CHECK(word(lock) == self);
	fast_release(lock);
	CHECK(word(lock) == 0);
	__rtos_lock_acquire(lock);
	CHECK(word(lock) == self);
	fast_release(lock);
	CHECK(word(lock) == 0);
	fast_acquire(lock);
	__rtos_lock_release(lock);
	CHECK(word(lock) == 0);

	/* Nested, it is marked slow until the last release */
	fast_acquire(lock);
	fast_acquire(lock);
	CHECK(word(lock) == (self | __RTOS_LOCK_SLOW));
	__rtos_lock_acquire(lock);
	CHECK(word(lock) == (self | __RTOS_LOCK_SLOW));
	fast_release(lock);
	__rtos_lock_release(lock);
	CHECK(word(lock) == (self | __RTOS_LOCK_SLOW));
	fast_release(lock);
	CHECK(word(lock) == 0);

	/* And taken by the fast path again afterwards */
	fast_acquire(lock);
	CHECK(word(lock) == self);
	fast_release(lock);

	__rtos_lock_destroy(lock);
}

static struct __rtos_lock *shared;
static unsigned long counter;
static unsigned int tokens[THREADS];

/* Each thread adds ROUNDS under the lock, sometimes nested, sometimes through the port, and yields while holding it */
static int contend(void *arg)
{
	unsigned int index = (unsigned int)(uintptr_t)arg;

	tokens[index] = __rtos_lock_token;
	for (unsigned int i = 0; i < ROUNDS; ++i) {
		bool nested = i % 3 == 0;
		bool port = (i + index) % 2 == 0;
		unsigned long seen;

		if (port)
			__rtos_lock_acquire(shared);
		else
			fast_acquire(shared);
		if (nested)
			fast_acquire(shared);

		seen = counter;
		if (i % 64 == 0)
			thrd_yield();
		counter = seen + 1;

		if (nested)
			fast_release(shared);
		if (port)
			fast_release(shared);
		else
			__rtos_lock_release(shared);
	}

	return 0;
}

static void check_contention(void)
{
	thrd_t threads[THREADS];

	CHECK(__rtos_lock_create(&shared) == 0);
	for (unsigned int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], contend, (void *)(uintptr_t)i) == thrd_success);
	for (unsigned int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], nullptr) == thrd_success);

	CHECK(counter == (unsigned long)THREADS * ROUNDS);
	CHECK(word(shared) == 0);

	/* Every thread had its own token, none of them the initial thread's */
	for (unsigned int i = 0; i < THREADS; ++i) {
		CHECK(tokens[i] != 0 && (tokens[i] & __RTOS_LOCK_SLOW) == 0 && tokens[i] != __rtos_lock_token);
		for (unsigned int k = 0; k < i; ++k)
			CHECK(tokens[i] != tokens[k]);
	}

	__rtos_lock_destroy(shared);
}

int main(void)
{
	check_states();
	check_contention();

	if (failures == 0)
		puts("pass: locks");

	return failures == 0 ? 0 : 1;
}
