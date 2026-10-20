/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/format.h"

struct output {
	struct __libc_format_sink *sink;
	size_t count;
};

static const char spaces[] = "                ";
static const char zeros[] = "0000000000000000";

/* Fails with EOVERFLOW when n more characters would take the result past INT_MAX */
static bool reserve(struct output *out, size_t n)
{
	if (n > INT_MAX - out->count) {
		errno = EOVERFLOW;
		return false;
	}

	return true;
}

static void emit(struct output *out, const char *s, size_t n)
{
	if (n == 0)
		return;

	out->sink->write(out->sink, s, n);
	out->count += n;
}

static void emit_fill(struct output *out, const char *fill, size_t n)
{
	while (n > 0) {
		size_t chunk = n < sizeof(spaces) - 1 ? n : sizeof(spaces) - 1;

		emit(out, fill, chunk);
		n -= chunk;
	}
}

/* Parses a decimal field; fails with EOVERFLOW above INT_MAX */
static bool parse_decimal(const char **format, int *value)
{
	const char *s = *format;
	int result = 0;

	while (*s >= '0' && *s <= '9') {
		int digit = *s++ - '0';

		if (result > (INT_MAX - digit) / 10) {
			errno = EOVERFLOW;
			return false;
		}
		result = result * 10 + digit;
	}

	*format = s;
	*value = result;
	return true;
}

/* C23 7.23.6.1p7 wN and wfN: the minimum-width and exact-width types of <stdint.h> */
static bool parse_width_length(const char **format, enum __libc_length *length)
{
	const char *s = *format;
	bool fast = false;
	int bits;

	if (*s == 'f') {
		fast = true;
		++s;
	}

	/* N is a positive decimal integer with no leading zeros */
	if (*s < '1' || *s > '9' || !parse_decimal(&s, &bits)) {
		errno = EINVAL;
		return false;
	}

	switch (bits) {
		case 8:
			*length = fast ? __LIBC_LENGTH_WF8 : __LIBC_LENGTH_W8;
			break;
		case 16:
			*length = fast ? __LIBC_LENGTH_WF16 : __LIBC_LENGTH_W16;
			break;
		case 32:
			*length = fast ? __LIBC_LENGTH_WF32 : __LIBC_LENGTH_W32;
			break;
		case 64:
			*length = fast ? __LIBC_LENGTH_WF64 : __LIBC_LENGTH_W64;
			break;
		default:
			/* C23 7.23.6.1p15: an unsupported width makes the call fail */
			errno = EINVAL;
			return false;
	}

	*format = s;
	return true;
}

static bool parse_length(const char **format, enum __libc_length *length)
{
	const char *s = *format;

	switch (*s++) {
		case 'h':
			*length = __LIBC_LENGTH_H;
			if (*s == 'h') {
				*length = __LIBC_LENGTH_HH;
				++s;
			}
			break;
		case 'l':
			*length = __LIBC_LENGTH_L;
			if (*s == 'l') {
				*length = __LIBC_LENGTH_LL;
				++s;
			}
			break;
		case 'j':
			*length = __LIBC_LENGTH_J;
			break;
		case 'z':
			*length = __LIBC_LENGTH_Z;
			break;
		case 't':
			*length = __LIBC_LENGTH_T;
			break;
		case 'L':
			*length = __LIBC_LENGTH_BIG_L;
			break;
		case 'w':
			if (!parse_width_length(&s, length))
				return false;
			break;
		default:
			*length = __LIBC_LENGTH_NONE;
			return true;
	}

	*format = s;
	return true;
}

/* C23 7.23.6.1p6 and p8: sign or base prefix, precision zeros, width padding */
static bool format_integer(struct output *out, const struct __libc_format_spec *spec, uintmax_t value, bool negative)
{
	char buffer[sizeof(uintmax_t) * CHAR_BIT];
	char *end = buffer + sizeof(buffer);
	char *digits = __libc_format_digits(end, value, spec->conversion);
	size_t ndigits = end - digits;
	size_t precision = spec->precision < 0 ? 1 : spec->precision;
	size_t nzeros = precision > ndigits ? precision - ndigits : 0;
	const char *prefix = "";
	size_t nprefix = 0;
	size_t length;
	size_t padding;

	switch (spec->conversion) {
		case 'd':
		case 'i':
			if (negative)
				prefix = "-";
			else if (spec->flags & __LIBC_FORMAT_PLUS)
				prefix = "+";
			else if (spec->flags & __LIBC_FORMAT_SPACE)
				prefix = " ";
			nprefix = *prefix != '\0';
			break;
		case 'o':
			/* The alternative form forces a leading zero, also for a zero value and precision */
			if ((spec->flags & __LIBC_FORMAT_ALT) && nzeros == 0 && (ndigits == 0 || *digits != '0'))
				nzeros = 1;
			break;
		case 'p':
			prefix = "0x";
			nprefix = 2;
			break;
		case 'x':
		case 'X':
		case 'b':
		case 'B':
			if ((spec->flags & __LIBC_FORMAT_ALT) && value != 0) {
				prefix = spec->conversion == 'x' ? "0x" : spec->conversion == 'X' ? "0X" :
													  spec->conversion == 'b'	  ? "0b" :
																					"0B";
				nprefix = 2;
			}
			break;
	}

	length = nprefix + nzeros + ndigits;
	padding = (size_t)spec->width > length ? spec->width - length : 0;
	if (!reserve(out, length + padding))
		return false;

	if (spec->flags & __LIBC_FORMAT_LEFT) {
		emit(out, prefix, nprefix);
		emit_fill(out, zeros, nzeros);
		emit(out, digits, ndigits);
		emit_fill(out, spaces, padding);
	} else if ((spec->flags & __LIBC_FORMAT_ZERO) && spec->precision < 0) {
		emit(out, prefix, nprefix);
		emit_fill(out, zeros, padding + nzeros);
		emit(out, digits, ndigits);
	} else {
		emit_fill(out, spaces, padding);
		emit(out, prefix, nprefix);
		emit_fill(out, zeros, nzeros);
		emit(out, digits, ndigits);
	}

	return true;
}

/* Writes n characters of s within the field width; the 0 flag does not apply */
static bool format_text(struct output *out, const struct __libc_format_spec *spec, const char *s, size_t n)
{
	size_t padding = (size_t)spec->width > n ? spec->width - n : 0;

	if (!reserve(out, n + padding))
		return false;

	if (!(spec->flags & __LIBC_FORMAT_LEFT))
		emit_fill(out, spaces, padding);
	emit(out, s, n);
	if (spec->flags & __LIBC_FORMAT_LEFT)
		emit_fill(out, spaces, padding);

	return true;
}

static bool format_string(struct output *out, const struct __libc_format_spec *spec, const char *s)
{
	size_t n = 0;

	/* A null pointer is undefined behaviour; print a marker instead of faulting */
	if (s == nullptr)
		s = spec->precision < 0 || spec->precision >= 6 ? "(null)" : "";

	/* With a precision, the array need not be null terminated (C23 7.23.6.1p8 s) */
	while ((spec->precision < 0 || n < (size_t)spec->precision) && s[n] != '\0')
		++n;

	return format_text(out, spec, s, n);
}

static bool format_float(struct output *out, const struct __libc_format_spec *spec, long double value)
{
	int status = __libc_format_float(out->sink, spec, value, out->count);

	if (status < 0)
		return false;
	out->count += status;

	return true;
}

/* Parses one conversion specification after the '%' and performs it */
static bool convert(struct output *out, const char **format, va_list *ap)
{
	struct __libc_format_spec spec = { .flags = 0, .width = 0, .precision = -1 };
	const char *s = *format;
	enum __libc_length length;
	uintmax_t magnitude;
	bool negative;
	char c;

	/* Flags, in any order */
	while (true) {
		if (*s == '-')
			spec.flags |= __LIBC_FORMAT_LEFT;
		else if (*s == '+')
			spec.flags |= __LIBC_FORMAT_PLUS;
		else if (*s == ' ')
			spec.flags |= __LIBC_FORMAT_SPACE;
		else if (*s == '#')
			spec.flags |= __LIBC_FORMAT_ALT;
		else if (*s == '0')
			spec.flags |= __LIBC_FORMAT_ZERO;
		else
			break;
		++s;
	}

	/* Field width; a negative * argument is a - flag and a positive width */
	if (*s == '*') {
		spec.width = va_arg(*ap, int);
		if (spec.width < 0) {
			if (spec.width == INT_MIN) {
				errno = EOVERFLOW;
				return false;
			}
			spec.flags |= __LIBC_FORMAT_LEFT;
			spec.width = -spec.width;
		}
		++s;
	} else if (!parse_decimal(&s, &spec.width)) {
		return false;
	}

	/* Precision; a lone period means zero and a negative * argument means omitted */
	if (*s == '.') {
		++s;
		if (*s == '*') {
			spec.precision = va_arg(*ap, int);
			if (spec.precision < 0)
				spec.precision = -1;
			++s;
		} else if (!parse_decimal(&s, &spec.precision)) {
			return false;
		}
	}

	if (!parse_length(&s, &length))
		return false;

	/* The 0 flag is ignored with the - flag */
	if (spec.flags & __LIBC_FORMAT_LEFT)
		spec.flags &= ~__LIBC_FORMAT_ZERO;

	c = *s++;
	spec.conversion = c;
	*format = s;

	switch (c) {
		case 'd':
		case 'i':
			if (length == __LIBC_LENGTH_BIG_L)
				break;
			/* Fetch first: the sign is only known afterwards */
			if (!__libc_format_argument(ap, length, true, &magnitude, &negative))
				return false;
			return format_integer(out, &spec, magnitude, negative);

		case 'b':
		case 'B':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			if (length == __LIBC_LENGTH_BIG_L)
				break;
			if (!__libc_format_argument(ap, length, false, &magnitude, &negative))
				return false;
			return format_integer(out, &spec, magnitude, false);

		case 'p':
			if (length != __LIBC_LENGTH_NONE)
				break;
			spec.precision = -1;
			spec.flags &= ~__LIBC_FORMAT_ZERO;
			return format_integer(out, &spec, (uintptr_t)va_arg(*ap, void *), false);

		case 'c':
			if (length == __LIBC_LENGTH_L) {
				/* Wide characters are out of scope (PLAN section 1) */
				errno = EILSEQ;
				return false;
			}
			if (length != __LIBC_LENGTH_NONE)
				break;
			c = (unsigned char)va_arg(*ap, int);
			return format_text(out, &spec, &c, 1);

		case 's':
			if (length == __LIBC_LENGTH_L) {
				errno = EILSEQ;
				return false;
			}
			if (length != __LIBC_LENGTH_NONE)
				break;
			return format_string(out, &spec, va_arg(*ap, const char *));

		case 'n':
			if (length == __LIBC_LENGTH_BIG_L)
				break;
			return __libc_format_count(ap, length, out->count);

		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			if (length == __LIBC_LENGTH_BIG_L)
				return format_float(out, &spec, va_arg(*ap, long double));
			if (length != __LIBC_LENGTH_NONE && length != __LIBC_LENGTH_L)
				break;
			return format_float(out, &spec, va_arg(*ap, double));

		case '%':
			if (!reserve(out, 1))
				return false;
			emit(out, "%", 1);
			return true;
	}

	/* An invalid conversion specification (C23 7.23.6.1p9) */
	errno = EINVAL;
	return false;
}

/* C23 7.23.6.1: the engine behind the printf family */
int __libc_format(struct __libc_format_sink *sink, const char *format, va_list ap)
{
	struct output out = { .sink = sink, .count = 0 };
	va_list args;
	int status = 0;

	/* A va_list parameter may be an array that decayed to a pointer, so work on a copy */
	va_copy(args, ap);

	while (*format != '\0') {
		const char *literal = format;

		while (*format != '\0' && *format != '%')
			++format;

		if (format != literal) {
			if (!reserve(&out, format - literal)) {
				status = -1;
				break;
			}
			emit(&out, literal, format - literal);
		}

		if (*format == '%') {
			++format;
			if (!convert(&out, &format, &args)) {
				status = -1;
				break;
			}
		}
	}

	va_end(args);

	return status < 0 ? status : (int)out.count;
}
