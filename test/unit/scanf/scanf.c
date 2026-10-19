/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * scanf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The scanf family (P1.10, docs/research/scanf.md): table-driven sscanf cases
 * (vectors.inc), typed modifiers whose sizes differ per target, long float
 * input against strtod, and fscanf on a file leaving the right input unread.
 */

static int failures;

static void fail(int line, const char *what)
{
	printf("fail: line %d: %s\n", line, what);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

#define MAX_VALUES 4

static const struct {
	const char *input;
	const char *format;
	const char *kinds;
	int result;
	const char *values[MAX_VALUES];
} vectors[] = {
#include "vectors.inc"
};

/* One output object per kind, initialised as the generator did, so untouched outputs compare too */
union output {
	int i;
	unsigned int u;
	long long ll;
	char s[64];
	char c[5];
	double d;
	float f;
};

static void prepare(union output *o, char kind)
{
	memset(o, 0, sizeof(*o));
	switch (kind) {
		case 'i':
		case 'n':
			o->i = -7777;
			break;
		case 'u':
			o->u = 7777;
			break;
		case 'L':
			o->ll = -7777;
			break;
		case 's':
			memset(o->s, '#', 63);
			break;
		case 'c':
			memset(o->c, '#', 4);
			break;
		case 'd':
			o->d = -7777.0;
			break;
		case 'f':
			o->f = -7777.0f;
			break;
	}
}

static void render(char *text, size_t size, const union output *o, char kind)
{
	uint64_t bits;
	uint32_t fbits;

	switch (kind) {
		case 'i':
		case 'n':
			snprintf(text, size, "%d", o->i);
			break;
		case 'u':
			snprintf(text, size, "%u", o->u);
			break;
		case 'L':
			snprintf(text, size, "%lld", o->ll);
			break;
		case 's':
			snprintf(text, size, "%s", o->s);
			break;
		case 'c':
			snprintf(text, size, "%.4s", o->c);
			break;
		case 'd':
			memcpy(&bits, &o->d, sizeof(bits));
			snprintf(text, size, "%016" PRIx64, bits);
			break;
		case 'f':
			memcpy(&fbits, &o->f, sizeof(fbits));
			snprintf(text, size, "%08" PRIx32, fbits);
			break;
	}
}

static void check_vectors(void)
{
	for (size_t v = 0; v < sizeof(vectors) / sizeof(vectors[0]); ++v) {
		union output out[MAX_VALUES];
		void *arg[MAX_VALUES];
		size_t n = strlen(vectors[v].kinds);
		char text[80];
		char message[256];
		int result;

		for (size_t k = 0; k < MAX_VALUES; ++k) {
			prepare(&out[k], k < n ? vectors[v].kinds[k] : 'i');
			arg[k] = &out[k];
		}
		result = sscanf(vectors[v].input, vectors[v].format, arg[0], arg[1], arg[2], arg[3]);
		if (result != vectors[v].result) {
			snprintf(message, sizeof(message), "sscanf(\"%s\", \"%s\") = %d, want %d", vectors[v].input, vectors[v].format, result, vectors[v].result);
			fail(__LINE__, message);
			continue;
		}
		for (size_t k = 0; k < n; ++k) {
			render(text, sizeof(text), &out[k], vectors[v].kinds[k]);
			if (strcmp(text, vectors[v].values[k]) != 0) {
				snprintf(message, sizeof(message), "sscanf(\"%s\", \"%s\") value %zu = \"%s\", want \"%s\"", vectors[v].input, vectors[v].format, k, text, vectors[v].values[k]);
				fail(__LINE__, message);
			}
		}
	}
}

/* Modifiers whose types differ in size between the targets */
static void check_typed(void)
{
	size_t z = 0;
	intmax_t j = 0;
	ptrdiff_t t = 0;
	uint_fast32_t fast = 0;
	int_least64_t least = 0;
	signed char hh = 0;
	long l = 0;
	void *p = nullptr;
	int local;
	char buffer[32];

	CHECK(sscanf("4000000000", "%zu", &z) == 1 && z == (size_t)4000000000u);
	CHECK(sscanf("-9223372036854775807", "%jd", &j) == 1 && j == -INTMAX_MAX);
	CHECK(sscanf("-12", "%td", &t) == 1 && t == -12);
	CHECK(sscanf("4294967295", "%wf32u", &fast) == 1 && fast == UINT32_MAX);
	CHECK(sscanf("-5000000000", "%w64d", &least) == 1 && least == -5000000000);
	CHECK(sscanf("-128", "%hhd", &hh) == 1 && hh == -128);
	CHECK(sscanf("-2147483648", "%ld", &l) == 1 && l == -2147483648L);

	/* %p reads back what %p printed */
	snprintf(buffer, sizeof(buffer), "%p", (void *)&local);
	CHECK(sscanf(buffer, "%p", &p) == 1 && p == &local);

	/* <inttypes.h> SCN macros */
	int32_t i32 = 0;
	uint16_t u16 = 0;

	CHECK(sscanf("-77 ffff", "%" SCNd32 " %" SCNx16, &i32, &u16) == 2 && i32 == -77 && u16 == 0xffff);

	/* An unknown conversion stops the scan */
	errno = 0;
	CHECK(sscanf("1 2", "%d %y", &local) == 1 && errno == EINVAL);
}

/* Float input of any length rounds as strtod does */
static void check_long_floats(void)
{
	static char text[3000];
	double d = 0;
	float f = 0;
	long double ld = 0;

	/* 2500 digits of .1111..., beyond the 900 the scanner keeps */
	memset(text, '1', sizeof(text) - 1);
	text[0] = '.';
	text[2500] = '\0';
	CHECK(sscanf(text, "%lf", &d) == 1 && d == strtod(text, nullptr));

	/* Leading zeros that do not count as digits, then a halfway case that the sticky digit decides */
	strcpy(text, "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000001e86");
	CHECK(sscanf(text, "%f", &f) == 1 && f == 1.0f);
	strcpy(text, "9007199254740993");
	CHECK(sscanf(text, "%lf", &d) == 1 && d == 9007199254740992.0);
	memset(text, '0', 1500);
	memcpy(text, "9007199254740993.", 17);
	text[1500] = '1';
	text[1501] = '\0';
	CHECK(sscanf(text, "%lf", &d) == 1 && d == 9007199254740994.0);
	CHECK(sscanf("0x1.fffffffffffff8p0", "%Lf", &ld) == 1 && ld == 2.0L);
	CHECK(sscanf("-0x.8p-1073", "%lf", &d) == 1 && d == -0x1p-1074);
	CHECK(sscanf("12345678901234567890e-10", "%lf", &d) == 1 && d == 1234567890.1234567890);

	/* Width limits the characters, sign included */
	CHECK(sscanf("-1.2345", "%4lf", &d) == 1 && d == -1.2);
}

static void check_stream(void)
{
	char name[L_tmpnam];
	char word[16];
	int a = 0;
	int b = 0;
	FILE *f;

	CHECK(tmpnam(name) == name);
	f = fopen(name, "w+");
	CHECK(f != nullptr);
	CHECK(fputs("10 20 thirty 0x 7\n", f) == 0);
	rewind(f);

	CHECK(fscanf(f, "%d%d", &a, &b) == 2 && a == 10 && b == 20);
	/* A failed %d leaves "thirty" for the next read */
	CHECK(fscanf(f, "%d", &a) == 0 && a == 10);
	CHECK(fscanf(f, "%15s", word) == 1 && strcmp(word, "thirty") == 0);
	/* "0x" is consumed and fails; the space after it stays unread */
	CHECK(fscanf(f, "%x", &a) == 0 && fgetc(f) == ' ');
	/* The caller's ungetc still works after a scan pushed a character back */
	CHECK(fscanf(f, "%d", &b) == 1 && b == 7 && ungetc('!', f) == '!' && fgetc(f) == '!' && fgetc(f) == '\n');
	CHECK(fscanf(f, "%d", &a) == EOF && feof(f));
	CHECK(fclose(f) == 0 && remove(name) == 0);
}

int main(void)
{
	check_vectors();
	check_typed();
	check_long_floats();
	check_stream();

	if (failures == 0)
		puts("pass: scanf");

	return failures == 0 ? 0 : 1;
}
