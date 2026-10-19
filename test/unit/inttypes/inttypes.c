/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * inttypes.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* <inttypes.h> (P1.05): every PRI macro family through snprintf, strtoimax/strtoumax, imaxabs, imaxdiv */

static int failures;

static void fail(int line, const char *what)
{
	char message[256];

	snprintf(message, sizeof(message), "fail: line %d: %s\n", line, what);
	fputs(message, stdout);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

static void expect(int line, const char *got, const char *want)
{
	if (strcmp(got, want) != 0) {
		char message[160];

		snprintf(message, sizeof(message), "got \"%s\", want \"%s\"", got, want);
		fail(line, message);
	}
}

#define EXPECT(want, format, ...)                              \
	do {                                                       \
		char buffer[128];                                      \
		snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
		expect(__LINE__, buffer, want);                        \
	} while (false)

int main(void)
{
	char *end;
	imaxdiv_t d;

	EXPECT("-128 127 -32768 -2147483648 -9223372036854775808", "%" PRId8 " %" PRIi8 " %" PRId16 " %" PRId32 " %" PRId64, (int8_t)INT8_MIN, (int8_t)INT8_MAX, (int16_t)INT16_MIN, (int32_t)INT32_MIN, (int64_t)INT64_MIN);
	EXPECT("255 177777 ffffffff FFFFFFFFFFFFFFFF", "%" PRIu8 " %" PRIo16 " %" PRIx32 " %" PRIX64, (uint8_t)UINT8_MAX, (uint16_t)UINT16_MAX, (uint32_t)UINT32_MAX, (uint64_t)UINT64_MAX);
	EXPECT("-1 -1 -1 -1", "%" PRIdLEAST8 " %" PRIdLEAST16 " %" PRIdLEAST32 " %" PRIdLEAST64, (int_least8_t)-1, (int_least16_t)-1, (int_least32_t)-1, (int_least64_t)-1);
	EXPECT("-1 -1 -1 -1", "%" PRIdFAST8 " %" PRIdFAST16 " %" PRIdFAST32 " %" PRIdFAST64, (int_fast8_t)-1, (int_fast16_t)-1, (int_fast32_t)-1, (int_fast64_t)-1);
	EXPECT("ff ffff", "%" PRIxFAST8 " %" PRIxLEAST16, (uint_fast8_t)0xff, (uint_least16_t)0xffff);
	EXPECT("-9223372036854775808 18446744073709551615", "%" PRIdMAX " %" PRIuMAX, INTMAX_MIN, UINTMAX_MAX);
	EXPECT("-5 4096", "%" PRIdPTR " %" PRIxPTR, (intptr_t)-5, (uintptr_t)0x4096);
	EXPECT("101 0B11", "%" PRIb8 " %#" PRIB16, (uint8_t)5, (uint16_t)3);

	errno = 0;
	CHECK(strtoimax(" -0x7fffffffffffffff", &end, 0) == -INTMAX_MAX && *end == '\0' && errno == 0);
	errno = 0;
	CHECK(strtoimax("-9223372036854775809", nullptr, 10) == INTMAX_MIN && errno == ERANGE);
	errno = 0;
	CHECK(strtoumax("0b11111111", nullptr, 0) == 255 && errno == 0);
	errno = 0;
	CHECK(strtoumax("18446744073709551616", nullptr, 10) == UINTMAX_MAX && errno == ERANGE);

	CHECK(imaxabs(-INTMAX_MAX) == INTMAX_MAX);
	d = imaxdiv(-17, 5);
	CHECK(d.quot == -3 && d.rem == -2);

	static_assert(__STDC_VERSION_INTTYPES_H__ == 202311L);

	if (failures == 0)
		puts("pass: inttypes");

	return failures == 0 ? 0 : 1;
}
