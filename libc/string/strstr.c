/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strstr.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stddef.h>
#include <string.h>

/*
 * C23 7.26.5.8 with the Two-Way algorithm (Crochemore and Perrin, 1991):
 * linear time and constant space. See docs/research/string-ctype-stdbit.md.
 */

/* Start of the maximal suffix of x[0..m) under one byte ordering; stores its period */
static size_t maximal_suffix(const unsigned char *x, size_t m, bool reverse, size_t *period)
{
	size_t start = 0;
	size_t candidate = 1;
	size_t offset = 1;
	size_t p = 1;

	while (candidate + offset <= m) {
		unsigned char a = x[candidate + offset - 1];
		unsigned char b = x[start + offset - 1];

		if (reverse ? a > b : a < b) {
			/* The current suffix stays maximal; the period grows to the candidate */
			candidate += offset;
			offset = 1;
			p = candidate - start;
		} else if (a == b) {
			/* Still matching the current suffix; step over whole periods */
			if (offset == p) {
				candidate += p;
				offset = 1;
			} else {
				++offset;
			}
		} else {
			/* The candidate is a larger suffix */
			start = candidate;
			candidate = start + 1;
			offset = 1;
			p = 1;
		}
	}

	*period = p;
	return start;
}

static const unsigned char *two_way(const unsigned char *h, size_t n, const unsigned char *x, size_t m)
{
	size_t period_less;
	size_t period_greater;
	size_t split_less = maximal_suffix(x, m, false, &period_less);
	size_t split_greater = maximal_suffix(x, m, true, &period_greater);
	size_t split = split_less > split_greater ? split_less : split_greater;
	size_t period = split_less > split_greater ? period_less : period_greater;
	size_t memory = 0;
	size_t pos = 0;
	bool periodic;

	/* A periodic needle repeats its left part one period later; then matched bytes can be remembered */
	periodic = split + period <= m && memcmp(x, x + period, split) == 0;
	if (!periodic)
		period = (split > m - split ? split : m - split) + 1;

	while (pos + m <= n) {
		size_t i = split > memory ? split : memory;

		/* Right part, left to right */
		while (i < m && x[i] == h[pos + i])
			++i;
		if (i < m) {
			pos += i - split + 1;
			memory = 0;
			continue;
		}

		/* Left part, right to left, down to what is already known to match */
		i = split;
		while (i > memory && x[i - 1] == h[pos + i - 1])
			--i;
		if (i <= memory)
			return h + pos;

		pos += period;
		memory = periodic ? m - period : 0;
	}

	return nullptr;
}

char *(strstr)(const char *s1, const char *s2)
{
	const char *start;

	if (s2[0] == '\0')
		return (char *)s1;

	/* Skip to the first possible match; a one-character needle is then found */
	start = strchr(s1, s2[0]);
	if (start == nullptr || s2[1] == '\0')
		return (char *)start;

	return (char *)two_way((const unsigned char *)start, strlen(start), (const unsigned char *)s2, strlen(s2));
}
