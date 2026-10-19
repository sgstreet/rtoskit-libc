/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_format_float.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdint.h>

#include "internal/decimal.h"
#include "internal/format.h"

/*
 * C23 7.23.6.1p8 a, A, e, E, f, F, g, G (docs/research/printf-float.md): the
 * value is expanded exactly into decimal, rounded once at the requested digit
 * in the current rounding direction (Annex F.5), and written as a sequence of
 * pieces, so long precisions need no buffer. long double is binary64 (PLAN D7).
 */

/* Precisions beyond this exceed every exact expansion, so they only add zeros */
#define EXACT_DIGITS (__LIBC_DECIMAL_DIGITS + 400)

#define FRACTION_BITS (DBL_MANT_DIG - 1)
#define EXPONENT_BIAS (DBL_MAX_EXP - 1)

/* A run of text, or length copies of fill when text is null */
struct piece {
	const char *text;
	size_t length;
	char fill;
};

struct pieces {
	struct piece piece[8];
	unsigned int count;
};

static const char zeros[] = "0000000000000000";
static const char spaces[] = "                ";

static void add_text(struct pieces *p, const char *text, size_t length)
{
	if (length > 0)
		p->piece[p->count++] = (struct piece){ .text = text, .length = length };
}

static void add_fill(struct pieces *p, char fill, size_t length)
{
	if (length > 0)
		p->piece[p->count++] = (struct piece){ .text = nullptr, .length = length, .fill = fill };
}

static void write_fill(struct __libc_format_sink *sink, const char *fill, size_t length)
{
	while (length > 0) {
		size_t chunk = length < sizeof(zeros) - 1 ? length : sizeof(zeros) - 1;

		sink->write(sink, fill, chunk);
		length -= chunk;
	}
}

/*
 * The sign or base prefix, the pieces, and the padding of the field width:
 * spaces before or after, or zeros after the prefix with the 0 flag (p6),
 * which does not apply to infinity and NaN.
 */
static int write_field(struct __libc_format_sink *sink, const struct __libc_format_spec *spec, size_t count, const char *prefix, size_t nprefix, const struct pieces *p, bool finite)
{
	size_t length = nprefix;
	size_t padding;

	for (unsigned int i = 0; i < p->count; ++i)
		length += p->piece[i].length;
	padding = (size_t)spec->width > length ? spec->width - length : 0;

	if (length + padding > INT_MAX - count) {
		errno = EOVERFLOW;
		return -1;
	}

	bool left = spec->flags & __LIBC_FORMAT_LEFT;
	bool zero_fill = !left && finite && (spec->flags & __LIBC_FORMAT_ZERO);

	if (!left && !zero_fill)
		write_fill(sink, spaces, padding);
	if (nprefix > 0)
		sink->write(sink, prefix, nprefix);
	if (zero_fill)
		write_fill(sink, zeros, padding);
	for (unsigned int i = 0; i < p->count; ++i) {
		if (p->piece[i].text != nullptr)
			sink->write(sink, p->piece[i].text, p->piece[i].length);
		else
			write_fill(sink, p->piece[i].fill == '0' ? zeros : spaces, p->piece[i].length);
	}
	if (left)
		write_fill(sink, spaces, padding);

	return (int)(length + padding);
}

/* An exponent with its sign and at least min_digits digits, written backwards from end */
static char *format_exponent(char *end, int exponent, int min_digits, char marker)
{
	char *p = end;
	unsigned int magnitude = exponent < 0 ? -(unsigned int)exponent : (unsigned int)exponent;

	do {
		*--p = '0' + magnitude % 10;
		magnitude /= 10;
		--min_digits;
	} while (magnitude != 0 || min_digits > 0);
	*--p = exponent < 0 ? '-' : '+';
	*--p = marker;

	return p;
}

/* p8 f, F: [-]ddd.ddd with precision digits after the point, from a rounded decimal in ASCII */
static void add_fixed(struct pieces *p, const struct __libc_decimal *d, int precision, bool point)
{
	int integer = d->point > 0 ? d->point : 0;
	int available = d->count < integer ? d->count : integer;

	if (integer == 0) {
		add_text(p, zeros, 1);
	} else {
		add_text(p, (const char *)d->digits, available);
		add_fill(p, '0', integer - available);
	}

	if (precision > 0 || point)
		add_text(p, ".", 1);
	if (precision <= 0)
		return;

	/* Zeros between the point and the first digit, the digits, then zeros to the precision */
	int leading = d->point < 0 ? -d->point : 0;

	if (d->count == 0 || leading >= precision) {
		add_fill(p, '0', precision);
		return;
	}
	add_fill(p, '0', leading);

	int first = d->point > 0 ? d->point : 0;
	int digits = d->count > first ? d->count - first : 0;

	add_text(p, (const char *)d->digits + first, digits);
	add_fill(p, '0', precision - leading - digits);
}

/* p8 e, E: [-]d.ddde±dd, from a rounded decimal in ASCII; exponent is written into buffer */
static void add_scientific(struct pieces *p, const struct __libc_decimal *d, int precision, bool point, char marker, char *buffer, size_t size)
{
	int exponent = d->count == 0 ? 0 : d->point - 1;
	char *end = buffer + size;
	char *text = format_exponent(end, exponent, 2, marker);

	add_text(p, d->count == 0 ? zeros : (const char *)d->digits, 1);
	if (precision > 0 || point)
		add_text(p, ".", 1);
	if (precision > 0) {
		int digits = d->count > 1 ? d->count - 1 : 0;

		add_text(p, (const char *)d->digits + 1, digits);
		add_fill(p, '0', precision - digits);
	}
	add_text(p, text, end - text);
}

/* Keep digits in [0, INT_MAX - 1000] so adding a decimal point position cannot overflow */
static int limit_digits(int digits)
{
	return digits > EXACT_DIGITS ? EXACT_DIGITS : digits;
}

/* p8 a, A: [-]0xh.hhhp±d, normalised to a leading 1 (subnormals too); the exponent is decimal */
static void add_hex(struct pieces *p, uint64_t bits, const struct __libc_format_spec *spec, int rounding, bool negative, char *buffer, size_t size)
{
	static const char lower[] = "0123456789abcdef";
	static const char upper[] = "0123456789ABCDEF";
	const char *hex = spec->conversion == 'A' ? upper : lower;
	uint64_t fraction = bits & (((uint64_t)1 << FRACTION_BITS) - 1);
	int field = (int)(bits >> FRACTION_BITS) & (2 * EXPONENT_BIAS + 1);
	int exponent = field - EXPONENT_BIAS;
	char leading = '1';
	int precision = spec->precision;
	char *end = buffer + size;

	if (field == 0 && fraction == 0) {
		leading = '0';
		exponent = 0;
	} else if (field == 0) {
		/* Subnormal: shift the leading 1 into the implicit position */
		exponent = 1 - EXPONENT_BIAS;
		while ((fraction & ((uint64_t)1 << FRACTION_BITS)) == 0) {
			fraction <<= 1;
			--exponent;
		}
		fraction &= ((uint64_t)1 << FRACTION_BITS) - 1;
	}

	/* 13 hexadecimal digits hold the 52 fraction bits exactly */
	int digits = FRACTION_BITS / 4;

	if (precision >= 0 && precision < digits) {
		unsigned int drop = 4 * (digits - precision);
		uint64_t rest = fraction & (((uint64_t)1 << drop) - 1);
		uint64_t half = (uint64_t)1 << (drop - 1);
		/* The last kept digit's parity; with no fraction digits it is the leading digit's */
		bool odd = ((fraction >> drop) | (precision == 0 && leading == '1')) & 1;
		bool increment;

		fraction >>= drop;
		switch (rounding) {
			case __LIBC_ROUND_TOWARD_ZERO:
				increment = false;
				break;
			case __LIBC_ROUND_UPWARD:
				increment = rest != 0 && !negative;
				break;
			case __LIBC_ROUND_DOWNWARD:
				increment = rest != 0 && negative;
				break;
			default:
				increment = rest > half || (rest == half && odd);
				break;
		}
		fraction += increment;

		/* 1.fff... rounding up to 2 renormalises */
		if (leading == '1' && fraction >> (4 * precision) != 0) {
			fraction = 0;
			++exponent;
		}
		digits = precision;
	} else if (precision < 0) {
		/* The exact value: drop trailing zero digits */
		while (digits > 0 && (fraction & 0xf) == 0) {
			fraction >>= 4;
			--digits;
		}
		precision = digits;
	}

	char *text = format_exponent(end, exponent, 1, spec->conversion == 'A' ? 'P' : 'p');
	char *q = text - digits;

	for (int i = digits - 1; i >= 0; --i) {
		q[i] = hex[fraction & 0xf];
		fraction >>= 4;
	}

	add_text(p, leading == '1' ? "1" : "0", 1);
	if (precision > 0 || (spec->flags & __LIBC_FORMAT_ALT))
		add_text(p, ".", 1);
	add_text(p, q, digits);
	add_fill(p, '0', precision - digits);
	add_text(p, text, end - text);
}

int __libc_format_float(struct __libc_format_sink *sink, const struct __libc_format_spec *spec, long double value, size_t count)
{
	struct __libc_decimal d;
	struct pieces p = { .count = 0 };
	char buffer[32];
	char prefix[3];
	size_t nprefix = 0;
	double v = value;
	uint64_t bits;
	char conversion = spec->conversion;
	bool upper = conversion == 'A' || conversion == 'E' || conversion == 'F' || conversion == 'G';
	bool alt = spec->flags & __LIBC_FORMAT_ALT;
	int precision = spec->precision;
	int rounding = FLT_ROUNDS;

	static_assert(sizeof(double) == sizeof(uint64_t) && LDBL_MANT_DIG == DBL_MANT_DIG);
	__builtin_memcpy(&bits, &v, sizeof(bits));

	bool negative = bits >> 63;
	int field = (int)(bits >> FRACTION_BITS) & (2 * EXPONENT_BIAS + 1);
	uint64_t fraction = bits & (((uint64_t)1 << FRACTION_BITS) - 1);

	/* The sign (p6: + and space flags), shown for negative zero and NaN too */
	if (negative)
		prefix[nprefix++] = '-';
	else if (spec->flags & __LIBC_FORMAT_PLUS)
		prefix[nprefix++] = '+';
	else if (spec->flags & __LIBC_FORMAT_SPACE)
		prefix[nprefix++] = ' ';

	if (field == 2 * EXPONENT_BIAS + 1) {
		const char *text = fraction != 0 ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");

		add_text(&p, text, 3);
		return write_field(sink, spec, count, prefix, nprefix, &p, false);
	}

	if (conversion == 'a' || conversion == 'A') {
		prefix[nprefix++] = '0';
		prefix[nprefix++] = upper ? 'X' : 'x';
		add_hex(&p, bits, spec, rounding, negative, buffer, sizeof(buffer));
		return write_field(sink, spec, count, prefix, nprefix, &p, true);
	}

	/* The exact magnitude: significand * 2^exponent */
	uint64_t significand = field == 0 ? fraction : fraction | (uint64_t)1 << FRACTION_BITS;
	int exponent = (field == 0 ? 1 : field) - EXPONENT_BIAS - FRACTION_BITS;

	__libc_decimal_set(&d, significand, exponent);

	if (precision < 0)
		precision = 6;

	bool scientific = conversion == 'e' || conversion == 'E';

	if (conversion == 'g' || conversion == 'G') {
		/* p8 g: P significant digits; the exponent X of the rounded value picks the style */
		int significant = precision == 0 ? 1 : precision;

		__libc_decimal_round(&d, limit_digits(significant), negative, rounding);

		int x = d.count == 0 ? 0 : d.point - 1;

		scientific = !(significant > x && x >= -4);
		precision = scientific ? significant - 1 : significant - 1 - x;

		/* Without #, trailing zeros go (the digits are already trimmed), and the point with them */
		if (!alt) {
			int kept = scientific ? d.count - 1 : d.count - d.point;

			precision = kept < 0 ? 0 : kept < precision ? kept :
														  precision;
		}
	} else if (scientific) {
		__libc_decimal_round(&d, limit_digits(precision) + 1, negative, rounding);
	} else {
		__libc_decimal_round(&d, d.point + limit_digits(precision), negative, rounding);
	}

	for (int i = 0; i < d.count; ++i)
		d.digits[i] += '0';

	if (scientific)
		add_scientific(&p, &d, precision, alt, upper ? 'E' : 'e', buffer, sizeof(buffer));
	else
		add_fixed(&p, &d, precision, alt);

	return write_field(sink, spec, count, prefix, nprefix, &p, true);
}
