/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ctype.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/*
 * <ctype.h> in the "C" locale (P1.03): every unsigned char value and EOF
 * against the character lists of C23 5.2.1 and 7.4, written out as strings
 * rather than ranges so the test does not share the implementation's
 * arithmetic.
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

static const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char lower[] = "abcdefghijklmnopqrstuvwxyz";
static const char digits[] = "0123456789";
static const char xdigits[] = "0123456789abcdefABCDEF";
static const char punct[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
static const char space[] = " \t\n\v\f\r";

static bool in(const char *set, int c)
{
	return c != 0 && memchr(set, c, strlen(set)) != nullptr;
}

static void check(int c)
{
	bool is_upper = in(upper, c);
	bool is_lower = in(lower, c);
	bool is_alpha = is_upper || is_lower;
	bool is_digit = in(digits, c);
	bool is_punct = in(punct, c);
	bool is_graph = is_alpha || is_digit || is_punct;
	bool is_print = is_graph || c == ' ';
	bool is_cntrl = (c >= 0 && c < 0x20) || c == 0x7f;
	char message[64];
	int before = failures;

	CHECK(!isupper(c) == !is_upper);
	CHECK(!islower(c) == !is_lower);
	CHECK(!isalpha(c) == !is_alpha);
	CHECK(!isdigit(c) == !is_digit);
	CHECK(!isalnum(c) == !(is_alpha || is_digit));
	CHECK(!isxdigit(c) == !in(xdigits, c));
	CHECK(!ispunct(c) == !is_punct);
	CHECK(!isgraph(c) == !is_graph);
	CHECK(!isprint(c) == !is_print);
	CHECK(!iscntrl(c) == !is_cntrl);
	CHECK(!isspace(c) == !in(space, c));
	CHECK(!isblank(c) == !(c == ' ' || c == '\t'));
	CHECK(toupper(c) == (is_lower ? upper[strchr(lower, c) - lower] : c));
	CHECK(tolower(c) == (is_upper ? lower[strchr(upper, c) - upper] : c));

	if (failures != before) {
		snprintf(message, sizeof(message), "      for character %d\n", c);
		fputs(message, stdout);
	}
}

int main(void)
{
	for (int c = 0; c <= 255; ++c)
		check(c);
	check(EOF);

	if (failures == 0)
		puts("pass: ctype");

	return failures == 0 ? 0 : 1;
}
