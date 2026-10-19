/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * freestanding.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <float.h>
#include <iso646.h>
#include <limits.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbit.h>
#include <stdbool.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <string.h>

/*
 * C23 4p6 (P1.17, PLAN D50): the freestanding headers and <string.h>'s
 * freestanding functions. The module links this file twice: as a normal test
 * through the port, and as freestanding-bare.elf with libc.a alone, so any
 * dependency of these functions on the RTOS seam (errno, locks, TLS) fails
 * the build. Nothing here may use a hosted facility, not even to report: the
 * result is the exit status of check().
 */

static int sum(int count, ...)
{
	va_list arguments;
	int total = 0;

	va_start(arguments, count);
	for (int i = 0; i < count; ++i)
		total += va_arg(arguments, int);
	va_end(arguments);

	return total;
}

int check(void);

int check(void)
{
	char buffer[32] = "freestanding";
	char copy[32];
	int failures = 0;
	int product;
	alignas(16) unsigned char aligned[16];

	/* <string.h>: memccpy, memchr, memcmp, memcpy, memmove, memset, memset_explicit, strcat, strchr, strcmp, strcpy, strcspn, strlen, strncat, strncmp, strncpy, strpbrk, strrchr, strspn, strstr */
	failures += strlen(buffer) != 12;
	failures += memcpy(copy, buffer, 13) != copy || memcmp(copy, buffer, 13) != 0;
	failures += memmove(copy + 1, copy, 12) != copy + 1 || copy[1] != 'f';
	failures += memset(copy, 'x', 4) != copy || copy[3] != 'x';
	failures += memset_explicit(copy, 0, 4) != copy || copy[3] != 0;
	failures += memchr(buffer, 'n', 12) != buffer + 7;
	failures += memccpy(copy, buffer, 'e', 12) != copy + 3;
	failures += strcpy(copy, "free") != copy || strcat(copy, "dom") != copy || strcmp(copy, "freedom") != 0;
	failures += strncpy(copy, "abc", 5) != copy || copy[4] != 0 || strncat(copy, "defg", 2) != copy || strcmp(copy, "abcde") != 0;
	failures += strncmp("abcd", "abce", 3) != 0 || strchr(buffer, 'd') != buffer + 8 || strrchr(buffer, 'e') != buffer + 3;
	failures += strcspn(buffer, "xyzs") != 4 || strspn(buffer, "fer") != 4 || strpbrk(buffer, "ad") != buffer + 6;
	failures += strstr(buffer, "stand") != buffer + 4 || strstr(buffer, "") != buffer;

	/* <stdbit.h>, <stdckdint.h>, <stddef.h>, <stdint.h>, <limits.h>, <float.h>, <stdbool.h>, <iso646.h>, <stdalign.h> */
	failures += stdc_leading_zeros_ui(1u) != UINT_WIDTH - 1 || stdc_count_ones(0xf0u) != 4 || stdc_bit_ceil(5u) != 8u;
	failures += !ckd_mul(&product, INT_MAX, 2) || ckd_add(&product, 40, 2) || product != 42;
	failures += offsetof(struct { char c; int i; }, i) != alignof(int) || sizeof(max_align_t) < sizeof(long double);
	failures += INT32_MAX != 2147483647 || UINT8_WIDTH != 8 || (int)sizeof(intptr_t) * CHAR_BIT != INTPTR_WIDTH;
	failures += FLT_RADIX != 2 || DBL_MANT_DIG != 53 || BOOL_MAX != 1 || true != 1;
	failures += not(1 bitand 1) or ((uintptr_t)aligned % 16) != 0;
	failures += sum(3, 1, 2, 3) != 6 || nullptr != NULL;

	return failures;
}

int main(void)
{
	return check() == 0 ? 0 : 1;
}
