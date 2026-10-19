/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * atomic.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

/*
 * <stdatomic.h> and the __atomic_* library functions (P1.12,
 * docs/research/threads.md): every generic function on integer, pointer and
 * structure types, pointer arithmetic in elements, the lock-free macros,
 * atomic_flag, and 8-byte and odd-sized atomics updated by several threads
 * (the library path on Arm and for odd sizes everywhere).
 */

#define THREADS 4
#define ROUNDS 20000

static int failures;

static void fail(int line, const char *what)
{
	printf("fail: line %d: %s\n", line, what);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

/* Three bytes: no lock-free size, so every operation is a library call */
struct triple {
	unsigned char bytes[3];
};

/* Twelve bytes, larger than any lock-free size */
struct wide {
	unsigned int a;
	unsigned int b;
	unsigned int c;
};

static void check_generic(void)
{
	atomic_int i;
	atomic_uchar uc = 250;
	atomic_short s = -5;
	atomic_ullong ull = 0;
	atomic_llong ll = -1;
	atomic_bool b = false;
	_Atomic struct triple t;
	_Atomic struct wide w = { 1, 2, 3 };
	struct wide expected = { 1, 2, 3 };
	int array[8];
	_Atomic(int *) p = &array[1];
	int value;

	atomic_init(&i, 5);
	CHECK(atomic_load(&i) == 5);
	atomic_store(&i, 6);
	CHECK(atomic_load_explicit(&i, memory_order_acquire) == 6);
	CHECK(atomic_exchange(&i, 7) == 6 && i == 7);
	value = 7;
	CHECK(atomic_compare_exchange_strong(&i, &value, 8) && i == 8);
	CHECK(!atomic_compare_exchange_strong(&i, &value, 9) && value == 8 && i == 8);
	while (!atomic_compare_exchange_weak(&i, &value, 10))
		continue;
	CHECK(i == 10);
	CHECK(atomic_fetch_add(&i, 5) == 10 && atomic_fetch_sub(&i, 3) == 15 && i == 12);
	CHECK(atomic_fetch_or(&i, 0x30) == 12 && atomic_fetch_and(&i, 0x3c) == 0x3c && atomic_fetch_xor(&i, 0xff) == 0x3c && i == 0xc3);

	/* Narrow types wrap in their own width; signed operands subtract */
	CHECK(atomic_fetch_add(&uc, 10) == 250 && uc == 4);
	CHECK(atomic_fetch_add(&s, -5) == -5 && s == -10);
	CHECK(atomic_fetch_sub_explicit(&ull, 1, memory_order_relaxed) == 0 && ull == UINT64_MAX);
	CHECK(atomic_fetch_add(&ll, -1) == -1 && ll == -2);
	CHECK(atomic_fetch_add(&ll, 0x100000000LL) == -2 && ll == 0xfffffffeLL);
	CHECK(atomic_exchange(&b, true) == false && atomic_load(&b));

	/* Pointers move by elements */
	CHECK(atomic_fetch_add(&p, 3) == &array[1] && atomic_load(&p) == &array[4]);
	CHECK(atomic_fetch_sub(&p, 2) == &array[4] && p == &array[2]);

	/* Structures */
	atomic_init(&t, ((struct triple){ { 1, 2, 3 } }));
	struct triple got = atomic_load(&t);

	CHECK(memcmp(got.bytes, "\1\2\3", 3) == 0);
	got = atomic_exchange(&t, ((struct triple){ { 4, 5, 6 } }));
	CHECK(memcmp(got.bytes, "\1\2\3", 3) == 0 && memcmp(atomic_load(&t).bytes, "\4\5\6", 3) == 0);
	CHECK(atomic_compare_exchange_strong(&w, &expected, ((struct wide){ 4, 5, 6 })));
	CHECK(!atomic_compare_exchange_strong(&w, &expected, ((struct wide){ 7, 8, 9 })) && expected.a == 4 && expected.c == 6);
	atomic_store(&w, ((struct wide){ 10, 11, 12 }));
	CHECK(atomic_load(&w).b == 11);
	CHECK(kill_dependency(i) == 0xc3);
}

static void check_lock_free(void)
{
	atomic_int i = 0;
	atomic_llong ll = 0;
	atomic_char c = 0;
	_Atomic struct triple t;
	_Atomic struct wide w;

	/* "Always" lock-free types say so at run time; library-only sizes say they are not */
	CHECK(ATOMIC_INT_LOCK_FREE != 2 || atomic_is_lock_free(&i));
	CHECK(ATOMIC_LLONG_LOCK_FREE != 2 || atomic_is_lock_free(&ll));
	CHECK(ATOMIC_CHAR_LOCK_FREE != 2 || atomic_is_lock_free(&c));
	CHECK(!atomic_is_lock_free(&t) && !atomic_is_lock_free(&w));
	CHECK(ATOMIC_BOOL_LOCK_FREE >= 1 && ATOMIC_POINTER_LOCK_FREE >= 1 && ATOMIC_CHAR8_T_LOCK_FREE >= 1 && ATOMIC_CHAR16_T_LOCK_FREE >= 1 && ATOMIC_CHAR32_T_LOCK_FREE >= 1 && ATOMIC_WCHAR_T_LOCK_FREE >= 1 && ATOMIC_SHORT_LOCK_FREE >= 1 && ATOMIC_LONG_LOCK_FREE >= 1);
	CHECK(sizeof(atomic_int_least64_t) == 8 && sizeof(atomic_char8_t) == 1 && sizeof(atomic_char16_t) == 2 && sizeof(atomic_char32_t) == 4);
	CHECK(sizeof(atomic_intptr_t) == sizeof(void *) && sizeof(atomic_size_t) == sizeof(size_t) && sizeof(atomic_intmax_t) == 8);
}

static void check_flag(void)
{
	atomic_flag flag = ATOMIC_FLAG_INIT;

	/* The macros, then the functions behind them */
	CHECK(!atomic_flag_test_and_set(&flag) && atomic_flag_test_and_set(&flag));
	atomic_flag_clear(&flag);
	CHECK(!(atomic_flag_test_and_set)(&flag) && (atomic_flag_test_and_set_explicit)(&flag, memory_order_acquire));
	(atomic_flag_clear)(&flag);
	CHECK(!atomic_flag_test_and_set_explicit(&flag, memory_order_relaxed));
	(atomic_flag_clear_explicit)(&flag, memory_order_release);
	CHECK(!atomic_flag_test_and_set(&flag));
	atomic_flag_clear_explicit(&flag, memory_order_seq_cst);

	(atomic_thread_fence)(memory_order_seq_cst);
	(atomic_signal_fence)(memory_order_acq_rel);
	atomic_thread_fence(memory_order_release);
	atomic_signal_fence(memory_order_seq_cst);
}

/* Shared state updated by every thread */

static atomic_ullong wide_counter;
static atomic_uint narrow_counter;
static atomic_ushort short_counter;
static _Atomic struct triple triple_counter;
static atomic_flag spin = ATOMIC_FLAG_INIT;
static unsigned long guarded;

static int hammer(void *)
{
	for (int i = 0; i < ROUNDS; ++i) {
		struct triple old = atomic_load(&triple_counter);
		struct triple next;

		atomic_fetch_add(&wide_counter, 0x100000001ULL);
		atomic_fetch_add_explicit(&narrow_counter, 1, memory_order_relaxed);
		atomic_fetch_add(&short_counter, 1);

		/* A 24-bit counter kept in three bytes, by compare-exchange */
		do {
			unsigned long n = old.bytes[0] | old.bytes[1] << 8 | (unsigned long)old.bytes[2] << 16;

			++n;
			next = (struct triple){ { (unsigned char)n, (unsigned char)(n >> 8), (unsigned char)(n >> 16) } };
		} while (!atomic_compare_exchange_weak(&triple_counter, &old, next));

		while (atomic_flag_test_and_set_explicit(&spin, memory_order_acquire))
			thrd_yield();
		++guarded;
		atomic_flag_clear_explicit(&spin, memory_order_release);
	}

	return 0;
}

static void check_threads(void)
{
	thrd_t threads[THREADS];
	unsigned long total = (unsigned long)THREADS * ROUNDS;

	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_create(&threads[i], hammer, nullptr) == thrd_success);
	for (int i = 0; i < THREADS; ++i)
		CHECK(thrd_join(threads[i], nullptr) == thrd_success);

	struct triple t = atomic_load(&triple_counter);

	CHECK(atomic_load(&wide_counter) == total * 0x100000001ULL);
	CHECK(narrow_counter == total && short_counter == (unsigned short)total);
	CHECK((t.bytes[0] | t.bytes[1] << 8 | (unsigned long)t.bytes[2] << 16) == total);
	CHECK(guarded == total);
}

int main(void)
{
	check_generic();
	check_lock_free();
	check_flag();
	check_threads();

	if (failures == 0)
		puts("pass: atomic");

	return failures == 0 ? 0 : 1;
}
