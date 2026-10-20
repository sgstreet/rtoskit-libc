/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * string_kernels.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Exhaustive alignment and length tests for the word-at-a-time string
 * kernels and their hand-written Arm versions (P2.06): every source and
 * destination offset modulo 8, every length up to 80 plus a few longer ones,
 * guard bytes on both sides, the return values; memmove in both directions
 * over every overlap; strlen and strcmp with bytes that trip a careless
 * zero-byte test (0x01, 0x80, 0xff) around the terminator and the
 * difference, compared as unsigned char (C23 7.26.4p1); strchr at every
 * offset, length and position, for characters that alias those bytes, the
 * terminator and int values beyond a byte. The references are plain byte
 * loops.
 */

#define SIZE 320
#define LIMIT 80

static int failures;
static unsigned char source[SIZE] __attribute__((aligned(8)));
static unsigned char target[SIZE] __attribute__((aligned(8)));
static unsigned char expected[SIZE] __attribute__((aligned(8)));

static const size_t long_lengths[] = { 127, 128, 129, 255, 256, 257 };

static void fail(const char *what, size_t a, size_t b, size_t n)
{
	char message[96];

	if (failures++ < 20) {
		snprintf(message, sizeof(message), "fail: %s offsets %zu, %zu length %zu\n", what, a, b, n);
		fputs(message, stdout);
	}
}

static void fill(unsigned char *buffer, unsigned seed)
{
	for (size_t i = 0; i < SIZE; ++i)
		buffer[i] = (unsigned char)(seed + i * 37 + (i >> 3));
}

static bool same(const unsigned char *a, const unsigned char *b, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		if (a[i] != b[i])
			return false;
	return true;
}

static void check_copy(size_t s, size_t d, size_t n)
{
	fill(source, 1);
	fill(target, 0x5a);
	fill(expected, 0x5a);
	for (size_t i = 0; i < n; ++i)
		expected[16 + d + i] = source[16 + s + i];
	if (memcpy(target + 16 + d, source + 16 + s, n) != target + 16 + d || !same(target, expected, SIZE))
		fail("memcpy", s, d, n);

	fill(target, 0x5a);
	if (memmove(target + 16 + d, source + 16 + s, n) != target + 16 + d || !same(target, expected, SIZE))
		fail("memmove (disjoint)", s, d, n);
}

/* memmove within one buffer: the reference copies through a temporary */
static void check_overlap(size_t s, size_t d, size_t n)
{
	unsigned char temporary[SIZE];

	fill(target, 3);
	fill(expected, 3);
	for (size_t i = 0; i < n; ++i)
		temporary[i] = expected[16 + s + i];
	for (size_t i = 0; i < n; ++i)
		expected[16 + d + i] = temporary[i];
	if (memmove(target + 16 + d, target + 16 + s, n) != target + 16 + d || !same(target, expected, SIZE))
		fail("memmove (overlap)", s, d, n);
}

static void check_set(size_t d, size_t n, int c)
{
	fill(target, 0x33);
	fill(expected, 0x33);
	for (size_t i = 0; i < n; ++i)
		expected[16 + d + i] = (unsigned char)c;
	if (memset(target + 16 + d, c, n) != target + 16 + d || !same(target, expected, SIZE))
		fail("memset", d, (size_t)c & 0x1ff, n);
}

/* A string of n non-zero bytes at offset d, cycling through values that look like zero bytes to a careless test */
static void make_string(unsigned char *buffer, size_t d, size_t n, unsigned seed)
{
	static const unsigned char tricky[] = { 0x01, 0x80, 0xff, 0x7f, 0x81, 0xfe, 0x41, 0x00 };

	for (size_t i = 0; i < SIZE; ++i)
		buffer[i] = tricky[(i + seed) % 7];
	buffer[16 + d + n] = '\0';
}

static void check_strlen(size_t d, size_t n)
{
	for (unsigned seed = 0; seed < 7; ++seed) {
		make_string(target, d, n, seed);
		if (strlen((const char *)target + 16 + d) != n)
			fail("strlen", d, seed, n);
	}
}

static int sign(int value)
{
	return (value > 0) - (value < 0);
}

static int reference_strcmp(const unsigned char *a, const unsigned char *b)
{
	while (*a != '\0' && *a == *b) {
		++a;
		++b;
	}

	return sign(*a - *b);
}

/* Equal strings, a difference at every position (each direction, across the sign bit), and one string a prefix of the other */
static void check_strcmp(size_t a, size_t b, size_t n)
{
	static const unsigned char pairs[][2] = { { 0x41, 0x42 }, { 0x7f, 0x80 }, { 0x01, 0xff }, { 0x80, 0x81 } };

	make_string(source, a, n, 2);
	make_string(target, b, n, 2);
	for (size_t i = 0; i < n; ++i)
		target[16 + b + i] = source[16 + a + i];
	if (strcmp((const char *)source + 16 + a, (const char *)target + 16 + b) != 0)
		fail("strcmp (equal)", a, b, n);

	for (size_t at = 0; at < n; ++at) {
		for (size_t k = 0; k < sizeof(pairs) / sizeof(pairs[0]); ++k) {
			unsigned char saved = target[16 + b + at];

			source[16 + a + at] = pairs[k][0];
			target[16 + b + at] = pairs[k][1];
			if (sign(strcmp((const char *)source + 16 + a, (const char *)target + 16 + b)) != reference_strcmp(source + 16 + a, target + 16 + b) || sign(strcmp((const char *)target + 16 + b, (const char *)source + 16 + a)) != reference_strcmp(target + 16 + b, source + 16 + a))
				fail("strcmp (difference)", a, b, n * 1000 + at);
			source[16 + a + at] = saved;
			target[16 + b + at] = saved;
		}
	}

	/* The shorter string ends where the other goes on */
	if (n > 0) {
		target[16 + b + n - 1] = '\0';
		if (sign(strcmp((const char *)source + 16 + a, (const char *)target + 16 + b)) != 1 || sign(strcmp((const char *)target + 16 + b, (const char *)source + 16 + a)) != -1)
			fail("strcmp (prefix)", a, b, n);
	}
}

static const unsigned char *reference_strchr(const unsigned char *s, unsigned char c)
{
	while (*s != c && *s != '\0')
		++s;
	return *s == c ? s : nullptr;
}

/* Every character position, characters that alias a careless zero test, the terminator, an absent character, and int values with high bits (converted to unsigned char, 7.26.5.2p2) */
static void check_strchr(size_t d, size_t n)
{
	static const int characters[] = { 0x01, 0x80, 0xff, 0x41, 0x7f, 0x141, -1, 0x55 };

	for (unsigned seed = 0; seed < 7; ++seed) {
		const char *text = (const char *)target + 16 + d;

		make_string(target, d, n, seed);
		for (size_t i = 0; i < sizeof(characters) / sizeof(characters[0]); ++i)
			if ((const unsigned char *)strchr(text, characters[i]) != reference_strchr(target + 16 + d, (unsigned char)characters[i]))
				fail("strchr", d, (size_t)characters[i] & 0x1ff, n);
		if (strchr(text, '\0') != text + n || strchr(text, 0x100) != text + n)
			fail("strchr (terminator)", d, seed, n);
		for (size_t at = 0; at < n; ++at) {
			unsigned char saved = target[16 + d + at];

			target[16 + d + at] = 0x55;
			if ((const unsigned char *)strchr(text, 0x55) != reference_strchr(target + 16 + d, 0x55))
				fail("strchr (position)", d, at, n);
			target[16 + d + at] = saved;
		}
	}
}

int main(void)
{
	static const int values[] = { 0, 0xff, 0x5a, 0x1a5, 0x15a, 0x7fffff00, -1 };

	for (size_t s = 0; s < 8; ++s) {
		for (size_t d = 0; d < 8; ++d) {
			for (size_t n = 0; n <= LIMIT; ++n)
				check_copy(s, d, n);
			for (size_t i = 0; i < sizeof(long_lengths) / sizeof(long_lengths[0]); ++i)
				check_copy(s, d, long_lengths[i]);
		}
	}
	/* Every overlap: source and destination 0 .. 23 bytes apart either way */
	for (size_t s = 0; s < 24; ++s)
		for (size_t d = 0; d < 24; ++d)
			for (size_t n = 0; n <= LIMIT; ++n)
				check_overlap(s, d, n);
	for (size_t d = 0; d < 8; ++d) {
		for (size_t n = 0; n <= LIMIT; ++n)
			for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
				check_set(d, n, values[i]);
		for (size_t i = 0; i < sizeof(long_lengths) / sizeof(long_lengths[0]); ++i)
			check_set(d, long_lengths[i], 0xa5);
	}
	for (size_t d = 0; d < 8; ++d)
		for (size_t n = 0; n <= LIMIT; ++n)
			check_strlen(d, n);
	for (size_t a = 0; a < 8; ++a)
		for (size_t b = 0; b < 8; ++b)
			for (size_t n = 0; n <= 40; ++n)
				check_strcmp(a, b, n);
	for (size_t d = 0; d < 8; ++d)
		for (size_t n = 0; n <= LIMIT; ++n)
			check_strchr(d, n);

	fputs(failures == 0 ? "string_kernels: pass\n" : "string_kernels: FAIL\n", stdout);
	return failures == 0 ? 0 : 1;
}
