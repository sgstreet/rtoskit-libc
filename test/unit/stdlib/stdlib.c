/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdlib.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <stdlib.h> part 2 (P1.05): the strtol family and its edge cases, ato*, abs
 * and div, qsort against an insertion sort, bsearch, rand, the "C" locale
 * multibyte functions and the environment. Vectors from
 * docs/research/stdlib-part2.md.
 */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static void check_strtol(int line, const char *s, int base, long want, ptrdiff_t end, int error)
{
	char *stop;
	long got;

	errno = 0;
	got = strtol(s, &stop, base);
	if (got != want || stop - s != end || errno != error) {
		char message[160];

		snprintf(message, sizeof(message), "strtol(\"%s\", %d) = %ld end %td errno %d", s, base, got, stop - s, errno);
		fail(line, message);
	}
}

#define STRTOL(s, base, want, end, error) check_strtol(__LINE__, s, base, want, end, error)

static void test_strtol(void)
{
	char text[64];
	char *stop;

	STRTOL("0", 10, 0, 1, 0);
	STRTOL("  -0x1fZ", 0, -31, 7, 0);
	STRTOL("\t\n+42abc", 10, 42, 5, 0);
	STRTOL("0x", 0, 0, 1, 0);
	STRTOL("0x", 16, 0, 1, 0);
	STRTOL("0xg", 16, 0, 1, 0);
	STRTOL("0b101", 0, 5, 5, 0);
	STRTOL("0b101", 2, 5, 5, 0);
	STRTOL("0B11", 0, 3, 4, 0);
	STRTOL("0b", 0, 0, 1, 0);
	STRTOL("0b2", 0, 0, 1, 0);
	STRTOL("0b101", 16, 0xb101, 5, 0);
	STRTOL("017", 0, 15, 3, 0);
	STRTOL("019", 0, 1, 2, 0);
	STRTOL("z", 36, 35, 1, 0);
	STRTOL("Zz", 36, 35 * 36 + 35, 2, 0);
	STRTOL("", 10, 0, 0, 0);
	STRTOL("+", 10, 0, 0, 0);
	STRTOL("-", 10, 0, 0, 0);
	STRTOL("   ", 10, 0, 0, 0);
	STRTOL("- 1", 10, 0, 0, 0);
	STRTOL("12", 1, 0, 0, EINVAL);
	STRTOL("12", 37, 0, 0, EINVAL);
	STRTOL("12", -1, 0, 0, EINVAL);

	snprintf(text, sizeof(text), "%ld", LONG_MAX);
	STRTOL(text, 10, LONG_MAX, (ptrdiff_t)strlen(text), 0);
	snprintf(text, sizeof(text), "%ld", LONG_MIN);
	STRTOL(text, 10, LONG_MIN, (ptrdiff_t)strlen(text), 0);
	snprintf(text, sizeof(text), "%luz", (unsigned long)LONG_MAX + 1);
	STRTOL(text, 10, LONG_MAX, (ptrdiff_t)strlen(text) - 1, ERANGE);
	snprintf(text, sizeof(text), "-%lu5", (unsigned long)LONG_MAX + 1);
	STRTOL(text, 10, LONG_MIN, (ptrdiff_t)strlen(text), ERANGE);
	STRTOL("99999999999999999999999999999999", 10, LONG_MAX, 32, ERANGE);

	errno = 0;
	CHECK(strtoul("-1", &stop, 10) == ULONG_MAX && errno == 0 && *stop == '\0');
	CHECK(strtoul("18446744073709551616", nullptr, 10) == ULONG_MAX || sizeof(long) == 4);
	errno = 0;
	CHECK(strtoull("18446744073709551616", nullptr, 10) == ULLONG_MAX && errno == ERANGE);
	errno = 0;
	CHECK(strtoull("-18446744073709551615", nullptr, 10) == 1 && errno == 0);
	errno = 0;
	CHECK(strtoll("-9223372036854775808", nullptr, 10) == LLONG_MIN && errno == 0);
	errno = 0;
	CHECK(strtoll("9223372036854775808", nullptr, 10) == LLONG_MAX && errno == ERANGE);
	CHECK(strtoul("ff", nullptr, 16) == 255);
	CHECK(strtol("777", nullptr, 8) == 511);
	CHECK(strtol("12", nullptr, 0) == 12);
	CHECK(strtol("11", (char **)nullptr, 2) == 3);

	CHECK(atoi("  -123x") == -123);
	CHECK(atol("2147483647") == 2147483647L);
	CHECK(atoll("-9223372036854775807") == -9223372036854775807LL);
	CHECK(atoi("0x10") == 0);
}

static void test_arithmetic(void)
{
	div_t d = div(7, -2);
	ldiv_t ld = ldiv(-7L, 2L);
	lldiv_t lld = lldiv(-7LL, -2LL);

	CHECK(abs(-5) == 5 && abs(5) == 5 && abs(0) == 0);
	CHECK(labs(-LONG_MAX) == LONG_MAX);
	CHECK(llabs(-LLONG_MAX) == LLONG_MAX);
	CHECK(abs(INT_MIN + 1) == INT_MAX);

	/* Truncation toward zero; quot * denom + rem == numer */
	CHECK(d.quot == -3 && d.rem == 1);
	CHECK(ld.quot == -3 && ld.rem == -1);
	CHECK(lld.quot == 3 && lld.rem == -1);
}

static int compare_int(const void *a, const void *b)
{
	int x = *(const int *)a;
	int y = *(const int *)b;

	return (x > y) - (x < y);
}

/* A three-byte element compared on its first byte only, to exercise byte swaps and duplicates */
static int compare_triple(const void *a, const void *b)
{
	return ((const unsigned char *)a)[0] - ((const unsigned char *)b)[0];
}

struct big {
	long key;
	char payload[17];
};

static int compare_big(const void *a, const void *b)
{
	long x = ((const struct big *)a)->key;
	long y = ((const struct big *)b)->key;

	return (x > y) - (x < y);
}

/* Comparisons counted, for the bound on the work qsort does */
static unsigned long comparisons;

static int compare_int_counted(const void *a, const void *b)
{
	++comparisons;
	return compare_int(a, b);
}

/* An int read from any alignment, for elements at an unaligned base */
static int compare_unaligned(const void *a, const void *b)
{
	int x;
	int y;

	memcpy(&x, a, sizeof(x));
	memcpy(&y, b, sizeof(y));
	return (x > y) - (x < y);
}

/*
 * McIlroy's adversary (A killer adversary for quicksort, 1999): every
 * element starts as "gas", and a value is fixed only when a comparison needs
 * one, choosing the pivot candidate so that a median-of-three quicksort
 * keeps partitioning off a few elements. The elements are indices into
 * adversary_value.
 */
#define ADVERSARY_COUNT 1000
static int adversary_value[ADVERSARY_COUNT];
static int adversary_solid;
static int adversary_candidate;

static int compare_adversary(const void *a, const void *b)
{
	int i = *(const int *)a;
	int j = *(const int *)b;
	int gas = ADVERSARY_COUNT;

	++comparisons;
	if (adversary_value[i] == gas && adversary_value[j] == gas) {
		if (i == adversary_candidate)
			adversary_value[i] = adversary_solid++;
		else
			adversary_value[j] = adversary_solid++;
	}
	if (adversary_value[i] == gas)
		adversary_candidate = i;
	else if (adversary_value[j] == gas)
		adversary_candidate = j;

	return adversary_value[i] - adversary_value[j];
}

/* 3 n log2 n: above the typical counts of a heapsort (2 n log2 n) and an introsort (about 1.4 n log2 n), far below n^2 */
static unsigned long comparison_bound(size_t n)
{
	unsigned long log = 0;

	for (size_t m = n; m > 1; m >>= 1)
		++log;
	return 3 * (unsigned long)n * (log + 1);
}

static void insertion_sort_int(int *values, size_t n)
{
	for (size_t i = 1; i < n; ++i)
		for (size_t j = i; j > 0 && values[j - 1] > values[j]; --j) {
			int t = values[j - 1];

			values[j - 1] = values[j];
			values[j] = t;
		}
}

/*
 * The flavours' sorts (P3.07, PLAN D90) against an insertion sort, element
 * for element: every size to 40 and larger ones across the introsort's
 * thresholds, five patterns, elements at an unaligned base; the comparison
 * count within the bound on 1000 elements of every pattern, and on McIlroy's
 * adversary, which only the depth limit's heapsort keeps from n^2.
 */
static void test_qsort_paths(void)
{
	static int values[1000];
	static int want[1000];
	static int input[1000];
	static unsigned char bytes[1000 * sizeof(int) + 1];
	static const size_t sizes[] = { 0, 1, 2, 3, 12, 13, 40, 64, 65, 100, 257, 1000 };
	uint32_t state = 11;

	for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]) + 41; ++s) {
		size_t n = s < 41 ? s : sizes[s - 41];

		for (int pattern = 0; pattern < 5; ++pattern) {
			bool same = true;

			for (size_t i = 0; i < n; ++i) {
				state = state * 1664525 + 1013904223;
				values[i] = pattern == 0 ? (int)(state >> 8) : pattern == 1 ? (int)i :
														   pattern == 2		? (int)(n - i) :
														   pattern == 3		? 7 :
																			  (int)(state >> 31);
				want[i] = values[i];
				input[i] = values[i];
			}
			insertion_sort_int(want, n);

			comparisons = 0;
			qsort(values, n, sizeof(int), compare_int_counted);
			for (size_t i = 0; i < n; ++i)
				same &= values[i] == want[i];
			CHECK(same);
			if (n == 1000)
				CHECK(comparisons <= comparison_bound(n));

			/* The same elements one byte past an aligned address: byte swaps */
			memcpy(bytes + 1, input, n * sizeof(int));
			qsort(bytes + 1, n, sizeof(int), compare_unaligned);
			CHECK(memcmp(bytes + 1, want, n * sizeof(int)) == 0);
		}
	}

	/* The adversary: elements are indices, values fixed as the sort asks */
	for (int i = 0; i < ADVERSARY_COUNT; ++i) {
		adversary_value[i] = ADVERSARY_COUNT;
		values[i] = i;
	}
	/*
	 * The first two elements are fixed as a descending pair, so a check for an
	 * input already in order (the speed flavour's) stops at once instead of
	 * letting the adversary answer every comparison as sorted
	 */
	adversary_value[0] = 1;
	adversary_value[1] = 0;
	adversary_solid = 2;
	adversary_candidate = 0;
	comparisons = 0;
	qsort(values, ADVERSARY_COUNT, sizeof(int), compare_adversary);
	/* An introsort spends up to 2 n log2 n in bad partitions before its heapsort's 2 n log2 n (Musser): 4/3 of the bound */
	CHECK(comparisons <= comparison_bound(ADVERSARY_COUNT) * 4 / 3);
	for (int i = 1; i < ADVERSARY_COUNT; ++i)
		CHECK(adversary_value[values[i - 1]] <= adversary_value[values[i]]);
}

static void test_qsort(void)
{
	static int values[1000];
	static unsigned char triples[300 * 3];
	static struct big bigs[100];
	uint32_t state = 7;

	qsort(nullptr, 0, sizeof(int), compare_int);
	values[0] = 4;
	qsort(values, 1, sizeof(int), compare_int);
	CHECK(values[0] == 4);

	for (size_t n = 0; n <= 1000; n = n < 20 ? n + 1 : n * 3 + 1) {
		for (int pattern = 0; pattern < 4; ++pattern) {
			bool sorted = true;

			for (size_t i = 0; i < n; ++i) {
				state = state * 1664525 + 1013904223;
				values[i] = pattern == 0 ? (int)(state >> 8) % 50 : pattern == 1 ? (int)i :
																pattern == 2	 ? (int)(n - i) :
																				   7;
			}
			qsort(values, n, sizeof(int), compare_int);
			for (size_t i = 1; i < n; ++i)
				sorted &= values[i - 1] <= values[i];
			CHECK(sorted);
		}
	}

	for (size_t i = 0; i < sizeof(triples); ++i) {
		state = state * 1664525 + 1013904223;
		triples[i] = (unsigned char)(state >> 24);
	}
	qsort(triples, 300, 3, compare_triple);
	for (size_t i = 1; i < 300; ++i)
		CHECK(triples[(i - 1) * 3] <= triples[i * 3]);

	for (size_t i = 0; i < 100; ++i) {
		bigs[i].key = (long)((i * 37) % 101);
		snprintf(bigs[i].payload, sizeof(bigs[i].payload), "k%ld", bigs[i].key);
	}
	qsort(bigs, 100, sizeof(bigs[0]), compare_big);
	for (size_t i = 0; i < 100; ++i) {
		char want[17];

		snprintf(want, sizeof(want), "k%ld", bigs[i].key);
		CHECK(strcmp(bigs[i].payload, want) == 0);
		CHECK(i == 0 || bigs[i - 1].key < bigs[i].key);
	}
}

static void test_bsearch(void)
{
	static const int sorted[] = { 1, 3, 5, 7, 9, 11 };
	int key;

	for (size_t i = 0; i < 6; ++i) {
		key = sorted[i];
		CHECK(bsearch(&key, sorted, 6, sizeof(int), compare_int) == &sorted[i]);
		key = sorted[i] + 1;
		CHECK(bsearch(&key, sorted, 6, sizeof(int), compare_int) == nullptr);
	}
	key = 0;
	CHECK(bsearch(&key, sorted, 6, sizeof(int), compare_int) == nullptr);
	CHECK(bsearch(&key, sorted, 0, sizeof(int), compare_int) == nullptr);

	/* C23 7.24.5.1p4: const in, const out */
	int mutable[] = { 1, 2 };

	static_assert(_Generic(bsearch(&key, sorted, 6, sizeof(int), compare_int), const void *: 1, default: 0));
	static_assert(_Generic(bsearch(&key, mutable, 2, sizeof(int), compare_int), void *: 1, default: 0));
}

static void test_rand(void)
{
	int first[8];
	int again[8];
	bool varied = false;

	/* Without srand the sequence is that of srand(1) (this runs first in the thread) */
	for (int i = 0; i < 8; ++i)
		first[i] = rand();
	srand(1);
	for (int i = 0; i < 8; ++i)
		again[i] = rand();
	CHECK(memcmp(first, again, sizeof(first)) == 0);

	srand(12345);
	for (int i = 0; i < 8; ++i)
		first[i] = rand();
	srand(12345);
	for (int i = 0; i < 8; ++i) {
		again[i] = rand();
		varied |= again[i] != first[0];
	}
	CHECK(memcmp(first, again, sizeof(first)) == 0);
	CHECK(varied);

	for (int i = 0; i < 100000; ++i) {
		int r = rand();

		if (r < 0 || r > RAND_MAX) {
			fail(__LINE__, "rand out of range");
			break;
		}
	}
	static_assert(RAND_MAX >= 32767);
}

static void test_multibyte(void)
{
	wchar_t wide[8];
	char narrow[8];
	wchar_t wc = 0;

	CHECK(MB_CUR_MAX == 1);
	CHECK(mblen(nullptr, 0) == 0);
	CHECK(mblen("a", 1) == 1 && mblen("", 1) == 0 && mblen("a", 0) == -1);
	CHECK(mbtowc(&wc, "\xe9", 1) == 1 && wc == 0xe9);
	CHECK(mbtowc(&wc, "", 1) == 0 && wc == 0);
	CHECK(mbtowc(nullptr, nullptr, 0) == 0);
	CHECK(wctomb(narrow, L'A') == 1 && narrow[0] == 'A');
	errno = 0;
	CHECK(wctomb(narrow, 0x100) == -1 && errno == EILSEQ);
	CHECK(wctomb(nullptr, L'A') == 0);
	CHECK(mbstowcs(wide, "abc", 8) == 3 && wide[0] == L'a' && wide[3] == L'\0');
	CHECK(mbstowcs(wide, "abcdef", 2) == 2);
	CHECK(wcstombs(narrow, L"xyz", 8) == 3 && strcmp(narrow, "xyz") == 0);
	CHECK(wcstombs(narrow, L"x\x1234", 8) == (size_t)-1);
}

static void test_environment(void)
{
	CHECK(getenv("RTOSKIT_LIBC_NO_SUCH_VARIABLE") == nullptr);
	CHECK(getenv("RTOSKIT_LIBC_TEST") != nullptr && strcmp(getenv("RTOSKIT_LIBC_TEST"), "value=with=equals") == 0);
	CHECK(getenv("RTOSKIT_LIBC_TES") == nullptr);
	CHECK(system(nullptr) == 0);
}

int main(void)
{
	test_rand();
	test_strtol();
	test_arithmetic();
	test_qsort();
	test_qsort_paths();
	test_bsearch();
	test_multibyte();
	test_environment();

	if (failures == 0)
		puts("pass: stdlib");

	return failures == 0 ? 0 : 1;
}
