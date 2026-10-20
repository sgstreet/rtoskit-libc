/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_assert_fail.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MESSAGE_SIZE 256

/* Appends text, truncating at the end of the buffer as snprintf would; a null text reads "(null)" (D28) */
static size_t append(char *message, size_t length, const char *text)
{
	if (text == nullptr)
		text = "(null)";

	while (*text != '\0' && length < MESSAGE_SIZE - 1)
		message[length++] = *text++;

	return length;
}

static size_t append_decimal(char *message, size_t length, int value)
{
	char digits[12];
	unsigned int magnitude = value < 0 ? -(unsigned int)value : (unsigned int)value;
	size_t count = sizeof(digits) - 1;

	digits[count] = '\0';
	do {
		digits[--count] = (char)('0' + magnitude % 10);
		magnitude /= 10;
	} while (magnitude != 0);
	if (value < 0)
		digits[--count] = '-';

	return append(message, length, &digits[count]);
}

/*
 * C23 7.2.2.1: report the failed assertion on the standard error stream, then
 * abort. The message is built by hand, as one write: snprintf would link the
 * whole formatter, floating point included, into every program that asserts
 * (PLAN D52).
 */
void __libc_assert_fail(const char *expression, const char *file, int line, const char *function)
{
	char message[MESSAGE_SIZE];
	size_t length = 0;

	length = append(message, length, file);
	length = append(message, length, ":");
	length = append_decimal(message, length, line);
	length = append(message, length, ": ");
	length = append(message, length, function);
	length = append(message, length, ": Assertion '");
	length = append(message, length, expression);
	length = append(message, length, "' failed.\n");
	message[length] = '\0';

	fputs(message, stderr);
	abort();
}
