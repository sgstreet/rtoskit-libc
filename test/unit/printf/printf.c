/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * printf.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Formatted-output core (P1.02): integer, character, string, pointer and %n
 * conversions, the C23 b/B and wN/wfN additions, and the snprintf contract.
 * The vectors come from docs/research/printf-engine.md.
 */

static int failures;

static size_t length(const char *s)
{
	size_t n = 0;

	while (s[n] != '\0')
		++n;

	return n;
}

static bool same(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b) {
		++a;
		++b;
	}

	return *a == *b;
}

static void fail(int line, const char *format, const char *got, const char *want)
{
	char message[512];

	snprintf(message, sizeof(message), "fail: line %d: \"%s\" gave \"%s\", want \"%s\"\n", line, format, got, want);
	fputs(message, stdout);
	++failures;
}

/* Checks the text and the return value of one snprintf call */
static void check(int line, const char *want, const char *format, ...)
{
	char buffer[256];
	va_list ap;
	int status;

	va_start(ap);
	status = vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	if (status < 0)
		fail(line, format, "<error>", want);
	else if (!same(buffer, want) || (size_t)status != length(want))
		fail(line, format, buffer, want);
}

/* Checks that a call fails with a negative result and the given errno */
static void check_error(int line, int error, const char *format, ...)
{
	char buffer[64];
	va_list ap;
	int status;

	errno = 0;
	va_start(ap);
	status = vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	if (status >= 0 || errno != error)
		fail(line, format, "<no error>", "<error>");
}

#define CHECK(want, ...) check(__LINE__, want, __VA_ARGS__)
#define CHECK_ERROR(error, ...) check_error(__LINE__, error, __VA_ARGS__)

static void test_integers(void)
{
	CHECK("0", "%d", 0);
	CHECK("-1", "%i", -1);
	CHECK("2147483647", "%d", INT_MAX);
	CHECK("-2147483648", "%d", INT_MIN);
	CHECK("4294967295", "%u", UINT_MAX);
	CHECK("", "%.0d", 0);
	CHECK("", "%.0x", 0);
	CHECK("    ", "%4.0u", 0);
	CHECK("00042", "%.5d", 42);
	CHECK("-00042", "%.5d", -42);

	/* Flags */
	CHECK("+5", "%+d", 5);
	CHECK(" 5", "% d", 5);
	CHECK("+5", "%+ d", 5);
	CHECK("-5", "% d", -5);
	CHECK("-0042", "%05d", -42);
	CHECK("-42  ", "%-05d", -42);
	CHECK("  007", "%05.3d", 7);
	CHECK("+0042", "%+05d", 42);
	CHECK("   42", "%5d", 42);
	CHECK("42   |", "%-5d|", 42);

	/* Alternative forms */
	CHECK("0", "%#o", 0);
	CHECK("0", "%#.0o", 0);
	CHECK("010", "%#o", 8);
	CHECK("010", "%#.3o", 8);
	CHECK("0", "%#x", 0);
	CHECK("0xff", "%#x", 255);
	CHECK("0XFF", "%#X", 255);
	CHECK("0x00ff", "%#06x", 255);
	CHECK("  0xff", "%#6x", 255);
	CHECK("0x000ff", "%#.5x", 255);
	CHECK("deadbeef", "%x", 0xdeadbeefU);
	CHECK("DEADBEEF", "%X", 0xdeadbeefU);
	CHECK("17777777777", "%o", INT_MAX);

	/* C23 binary conversions */
	CHECK("101", "%b", 5);
	CHECK("0b101", "%#b", 5);
	CHECK("0B101", "%#B", 5);
	CHECK("0", "%#b", 0);
	CHECK("00000101", "%08b", 5);
	CHECK("11111111111111111111111111111111", "%b", UINT_MAX);

	/* Star width and precision */
	CHECK("  42", "%*d", 4, 42);
	CHECK("42  |", "%*d|", -4, 42);
	CHECK("042", "%.*d", 3, 42);
	CHECK("42", "%.*d", -1, 42);
	CHECK("0", "%.*d", -1, 0);
	CHECK("   042", "%*.*d", 6, 3, 42);
}

static void test_lengths(void)
{
	CHECK("44", "%hhd", 300);
	CHECK("-1", "%hhd", 255);
	CHECK("255", "%hhu", -1);
	CHECK("1", "%hd", 65537);
	CHECK("65535", "%hu", -1);
	CHECK("-9223372036854775808", "%lld", LLONG_MIN);
	CHECK("9223372036854775807", "%lld", LLONG_MAX);
	CHECK("18446744073709551615", "%llu", ULLONG_MAX);
	CHECK("1777777777777777777777", "%llo", ULLONG_MAX);
	CHECK("ffffffffffffffff", "%llx", ULLONG_MAX);
	CHECK("1111111111111111111111111111111111111111111111111111111111111111", "%llb", ULLONG_MAX);
	CHECK("-9223372036854775808", "%jd", INTMAX_MIN);
	CHECK("18446744073709551615", "%ju", UINTMAX_MAX);
	CHECK("-1", "%ld", -1L);
	CHECK("123", "%zu", (size_t)123);
	CHECK("-123", "%zd", (ptrdiff_t)-123);
	CHECK("-7", "%td", (ptrdiff_t)-7);
	CHECK("4294967296", "%llu", 4294967296ULL);
	CHECK("10000000000", "%llu", 10000000000ULL);

	/* C23 exact-width and fastest minimum-width modifiers */
	CHECK("-128", "%w8d", (int8_t)-128);
	CHECK("44", "%w8d", 300);
	CHECK("255", "%w8u", (uint8_t)255);
	CHECK("ffff", "%w16x", (uint16_t)0xffff);
	CHECK("-32768", "%w16d", (int16_t)-32768);
	CHECK("4294967295", "%w32u", (uint32_t)UINT32_MAX);
	CHECK("-2147483648", "%w32d", (int32_t)INT32_MIN);
	CHECK("-9223372036854775808", "%w64d", (int64_t)INT64_MIN);
	CHECK("18446744073709551615", "%w64u", (uint64_t)UINT64_MAX);
	CHECK("-1", "%wf8d", (int_fast8_t)-1);
	CHECK("-1", "%wf16d", (int_fast16_t)-1);
	CHECK("-1", "%wf32d", (int_fast32_t)-1);
	CHECK("-1", "%wf64d", (int_fast64_t)-1);
	CHECK("ff", "%wf8x", (uint_fast8_t)255);
	CHECK("ffffffffffffffff", "%wf64x", (uint_fast64_t)UINT64_MAX);
}

static void test_text(void)
{
	char unterminated[3] = { 'a', 'b', 'c' };

	CHECK("x", "%c", 'x');
	CHECK("    x", "%5c", 'x');
	CHECK("x    |", "%-5c|", 'x');
	CHECK("\xff", "%c", 255);
	CHECK("hello", "%s", "hello");
	CHECK("abc", "%.3s", "abcdef");
	CHECK("ab", "%.2s", unterminated);
	CHECK("abc", "%.3s", unterminated);
	CHECK("   ab", "%5.2s", "abcdef");
	CHECK("ab   |", "%-5.2s|", "abcdef");
	CHECK("", "%s", "");
	CHECK("%", "%%");
	CHECK("100%", "%d%%", 100);
	CHECK("a%b", "a%%b");
	CHECK("plain text", "plain text");
	CHECK("", "");
}

static void test_pointers(void)
{
	char buffer[64];
	char want[64];
	int value;
	uintptr_t address = (uintptr_t)&value;
	int status;

	CHECK("0x0", "%p", (void *)nullptr);
	CHECK("  0x0", "%5p", (void *)nullptr);
	CHECK("0x0  |", "%-5p|", (void *)nullptr);
	CHECK("0x1234", "%p", (void *)0x1234);

	/* The same digits as %#jx for a non-null pointer */
	status = snprintf(buffer, sizeof(buffer), "%p", (void *)&value);
	snprintf(want, sizeof(want), "%#jx", (uintmax_t)address);
	if (status != (int)length(want) || !same(buffer, want))
		fail(__LINE__, "%p", buffer, want);
}

static void test_count(void)
{
	signed char hh = 0;
	short h = 0;
	int n = 0;
	long l = 0;
	long long ll = 0;
	intmax_t j = 0;
	size_t z = 0;
	ptrdiff_t t = 0;
	int8_t w8 = 0;
	int16_t w16 = 0;
	int32_t w32 = 0;
	int64_t w64 = 0;
	int_fast8_t wf8 = 0;
	int_fast16_t wf16 = 0;
	int_fast32_t wf32 = 0;
	int_fast64_t wf64 = 0;
	char buffer[64];
	/* A volatile size keeps the compiler from flagging the intended truncation */
	size_t volatile two = 2;

	snprintf(buffer, sizeof(buffer), "abc%nde%hhn%hn%ln%lln%jn%zn%tn", &n, &hh, &h, &l, &ll, &j, &z, &t);
	if (n != 3 || hh != 5 || h != 5 || l != 5 || ll != 5 || j != 5 || z != 5 || t != 5)
		fail(__LINE__, "%n", buffer, "counts of 3 and 5");

	snprintf(buffer, sizeof(buffer), "%5d%w8n%w16n%w32n%w64n%wf8n%wf16n%wf32n%wf64n", 1, &w8, &w16, &w32, &w64, &wf8, &wf16, &wf32, &wf64);
	if (w8 != 5 || w16 != 5 || w32 != 5 || w64 != 5 || wf8 != 5 || wf16 != 5 || wf32 != 5 || wf64 != 5)
		fail(__LINE__, "%wNn", buffer, "counts of 5");

	/* The count includes characters that did not fit */
	snprintf(buffer, two, "abcdef%n", &n);
	if (n != 6)
		fail(__LINE__, "%n", buffer, "count of 6");
}

static void test_snprintf(void)
{
	char buffer[8] = "xxxxxxx";
	size_t volatile one = 1;
	size_t volatile four = 4;
	int status;

	status = snprintf(nullptr, 0, "%d", 12345);
	if (status != 5)
		fail(__LINE__, "snprintf(nullptr, 0)", "wrong length", "5");

	status = snprintf(buffer, 0, "%d", 12345);
	if (status != 5 || buffer[0] != 'x')
		fail(__LINE__, "snprintf(buffer, 0)", buffer, "untouched");

	status = snprintf(buffer, one, "%d", 12345);
	if (status != 5 || buffer[0] != '\0' || buffer[1] != 'x')
		fail(__LINE__, "snprintf(buffer, 1)", buffer, "");

	status = snprintf(buffer, four, "%d", 12345);
	if (status != 5 || !same(buffer, "123") || buffer[4] != 'x')
		fail(__LINE__, "snprintf(buffer, 4)", buffer, "123");

	status = snprintf(buffer, sizeof(buffer), "%d", 1234567);
	if (status != 7 || !same(buffer, "1234567"))
		fail(__LINE__, "snprintf(buffer, 8)", buffer, "1234567");

	status = sprintf(buffer, "%s-%d", "ab", 42);
	if (status != 5 || !same(buffer, "ab-42"))
		fail(__LINE__, "sprintf", buffer, "ab-42");
}

static void test_errors(void)
{
	/* Non-literal formats so the compiler's format checking does not reject them */
	const char *volatile w7 = "%w7d";
	const char *volatile w08 = "%w08d";
	const char *volatile w128 = "%w128d";
	const char *volatile wf0 = "%wf0d";
	const char *volatile bad = "%y";
	const char *volatile wide_c = "%lc";
	const char *volatile wide_s = "%ls";
	const char *volatile huge_width = "%2147483648d";
	const char *volatile huge_total = "%2147483647d%d";

	CHECK_ERROR(EINVAL, w7, 1);
	CHECK_ERROR(EINVAL, w08, 1);
	CHECK_ERROR(EINVAL, w128, 1);
	CHECK_ERROR(EINVAL, wf0, 1);
	CHECK_ERROR(EINVAL, bad, 1);
	CHECK_ERROR(EILSEQ, wide_c, 'a');
	CHECK_ERROR(EILSEQ, wide_s, nullptr);
	CHECK_ERROR(EOVERFLOW, huge_width, 1);
	CHECK_ERROR(EOVERFLOW, huge_total, 1, 2);
	CHECK_ERROR(EOVERFLOW, "%*d", INT_MIN, 1);
}

int main(void)
{
	test_integers();
	test_lengths();
	test_text();
	test_pointers();
	test_count();
	test_snprintf();
	test_errors();

	if (failures == 0)
		puts("pass: printf");

	return failures == 0 ? 0 : 1;
}
