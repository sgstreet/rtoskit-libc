/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdio.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <stdio.h> streams (P1.09, docs/research/stdio.md). Without arguments: files
 * through every access function, positioning around buffered data and
 * pushback, buffering modes, the indicators, and the operations on files.
 * stdio.run drives the modes that need the standard streams: exit flushing,
 * perror, stdin from a pipe, and line buffering before input.
 */

static int failures;

static void fail(int line, const char *what)
{
	fprintf(stdout, "fail: line %d: %s\n", line, what);
	++failures;
}

#define CHECK(condition) ((condition) ? (void)0 : fail(__LINE__, #condition))

/* A fresh temporary name; the file is removed by the caller */
static char *temporary(char *name)
{
	char *result = tmpnam(name);

	CHECK(result == name);
	return name;
}

static void check_write_read(void)
{
	char name[L_tmpnam];
	char line[64];
	FILE *f;

	temporary(name);

	/* "w" creates, "r" reads back what was written */
	f = fopen(name, "w");
	CHECK(f != nullptr);
	CHECK(fputs("hello, world\n", f) >= 0);
	CHECK(fprintf(f, "%d %s %.2f\n", 42, "fortytwo", 4.25) == 17);
	CHECK(fputc('x', f) == 'x' && putc('y', f) == 'y');
	CHECK(fwrite("\nzz", 1, 3, f) == 3);
	CHECK(fclose(f) == 0);

	f = fopen(name, "r");
	CHECK(f != nullptr);
	CHECK(fgets(line, sizeof(line), f) == line && strcmp(line, "hello, world\n") == 0);
	CHECK(fgets(line, sizeof(line), f) == line && strcmp(line, "42 fortytwo 4.25\n") == 0);
	CHECK(fgetc(f) == 'x' && getc(f) == 'y');
	/* The last line has no new-line: fgets stops at end of file with what it has */
	CHECK(fgets(line, sizeof(line), f) == line && strcmp(line, "\n") == 0);
	/* Reading the last line ran into the end of the file */
	CHECK(fgets(line, sizeof(line), f) == line && strcmp(line, "zz") == 0);
	CHECK(feof(f));
	strcpy(line, "unchanged");
	CHECK(fgets(line, sizeof(line), f) == nullptr && strcmp(line, "unchanged") == 0);
	CHECK(feof(f) && !ferror(f));
	/* The end-of-file indicator is sticky until cleared (C23 7.23.7.1p3) */
	CHECK(fgetc(f) == EOF);
	clearerr(f);
	CHECK(!feof(f));
	/* Writing to an input-only stream sets the error indicator */
	CHECK(fputc('a', f) == EOF && ferror(f));
	CHECK(fclose(f) == 0);

	/* "a" appends; "r+" reads and writes */
	f = fopen(name, "a");
	CHECK(f != nullptr && fputs("tail", f) == 0 && fclose(f) == 0);
	f = fopen(name, "r+");
	CHECK(f != nullptr);
	CHECK(fseek(f, -4, SEEK_END) == 0 && fgets(line, sizeof(line), f) == line && strcmp(line, "tail") == 0);
	CHECK(fseek(f, 0, SEEK_SET) == 0 && fputs("HELLO", f) == 0);
	/* Output then input with a positioning call between (7.23.5.3p7) */
	CHECK(fseek(f, 0, SEEK_CUR) == 0 && fgets(line, sizeof(line), f) == line && strcmp(line, ", world\n") == 0);
	rewind(f);
	CHECK(fgets(line, 6, f) == line && strcmp(line, "HELLO") == 0);
	CHECK(fclose(f) == 0);

	/* "x": exclusive creation fails when the file exists */
	errno = 0;
	CHECK(fopen(name, "wx") == nullptr && errno == EEXIST);
	CHECK(remove(name) == 0);
	f = fopen(name, "w+bx");
	CHECK(f != nullptr && fclose(f) == 0);
	CHECK(remove(name) == 0);

	/* Invalid modes and missing files */
	errno = 0;
	CHECK(fopen(name, "q") == nullptr && errno == EINVAL);
	CHECK(fopen(name, "rx") == nullptr);
	CHECK(fopen(name, "r++") == nullptr);
	errno = 0;
	CHECK(fopen(name, "r") == nullptr && errno == ENOENT);
}

static void check_positioning(void)
{
	char name[L_tmpnam];
	fpos_t mark;
	FILE *f;

	f = fopen(temporary(name), "w+");
	CHECK(f != nullptr);

	/* Pending output counts in the position */
	CHECK(fputs("0123456789", f) == 0);
	CHECK(ftell(f) == 10);
	CHECK(fgetpos(f, &mark) == 0);
	CHECK(fputs("abc", f) == 0 && ftell(f) == 13);

	/* Unread input does not, and pushback moves the position back by one */
	CHECK(fseek(f, 2, SEEK_SET) == 0);
	CHECK(fgetc(f) == '2' && ftell(f) == 3);
	CHECK(ungetc('X', f) == 'X' && ftell(f) == 2);
	/* Only one character of pushback is guaranteed; this library keeps exactly one */
	CHECK(ungetc('Y', f) == EOF);
	CHECK(fgetc(f) == 'X' && fgetc(f) == '3' && ftell(f) == 4);

	/* SEEK_CUR is relative to the position the stream shows */
	CHECK(fseek(f, 3, SEEK_CUR) == 0 && fgetc(f) == '7');
	CHECK(fsetpos(f, &mark) == 0 && fgetc(f) == 'a');

	/* Positioning drops pushback and clears end of file */
	CHECK(fseek(f, 0, SEEK_END) == 0 && fgetc(f) == EOF && feof(f));
	CHECK(ungetc('Z', f) == 'Z' && !feof(f) && fgetc(f) == 'Z');
	CHECK(ungetc('Z', f) == 'Z' && fseek(f, 0, SEEK_SET) == 0 && fgetc(f) == '0');
	CHECK(ungetc(EOF, f) == EOF);

	errno = 0;
	CHECK(fseek(f, 0, 42) == -1 && errno == EINVAL);
	CHECK(fseek(f, -1, SEEK_SET) == -1 && errno == EINVAL);

	CHECK(fclose(f) == 0 && remove(name) == 0);
}

static void check_direct(void)
{
	char name[L_tmpnam];
	uint32_t out[300];
	uint32_t in[301];
	FILE *f;

	for (unsigned int i = 0; i < 300; ++i)
		out[i] = i * 2654435761u;

	f = fopen(temporary(name), "wb+");
	CHECK(f != nullptr);
	/* Larger than the buffer, so the write bypasses it */
	CHECK(fwrite(out, sizeof(out[0]), 300, f) == 300);
	CHECK(fwrite(out, 0, 5, f) == 0 && fwrite(out, 4, 0, f) == 0);
	rewind(f);
	/* Asking for more elements than exist returns the complete ones only */
	CHECK(fread(in, sizeof(in[0]), 301, f) == 300 && feof(f));
	CHECK(memcmp(in, out, sizeof(out)) == 0);
	/* A partial element is not counted */
	CHECK(fseek(f, -6, SEEK_END) == 0 && fread(in, 4, 2, f) == 1);
	CHECK(fclose(f) == 0 && remove(name) == 0);
}

static void check_buffering(void)
{
	char name[L_tmpnam];
	char buffer[64];
	char line[32];
	FILE *f;
	FILE *g;

	temporary(name);

	/* A full buffer of our own: nothing reaches the file until it is flushed */
	f = fopen(name, "w");
	CHECK(f != nullptr);
	CHECK(setvbuf(f, buffer, _IOFBF, sizeof(buffer)) == 0);
	CHECK(fputs("buffered\n", f) == 0);
	g = fopen(name, "r");
	CHECK(g != nullptr && fgetc(g) == EOF);
	CHECK(fflush(f) == 0);
	/* The file has grown, but the end-of-file indicator holds until cleared */
	CHECK(fgetc(g) == EOF);
	clearerr(g);
	CHECK(fgets(line, sizeof(line), g) == line && strcmp(line, "buffered\n") == 0);
	CHECK(fclose(g) == 0);

	/* setvbuf is refused while the stream holds data */
	CHECK(fputc('!', f) == '!' && setvbuf(f, nullptr, _IONBF, 0) != 0);
	CHECK(fclose(f) == 0);

	/* Line buffered: the new-line sends the line */
	f = fopen(name, "w");
	CHECK(f != nullptr && setvbuf(f, nullptr, _IOLBF, 128) == 0);
	CHECK(fputs("one", f) == 0);
	g = fopen(name, "r");
	CHECK(g != nullptr && fgetc(g) == EOF);
	CHECK(fputs(" two\nthree", f) == 0);
	clearerr(g);
	CHECK(fgets(line, sizeof(line), g) == line && strcmp(line, "one two\n") == 0);
	CHECK(fclose(g) == 0);
	CHECK(fclose(f) == 0);

	/* Unbuffered: every write goes straight through */
	f = fopen(name, "w");
	CHECK(f != nullptr);
	setbuf(f, nullptr);
	CHECK(fputc('u', f) == 'u');
	g = fopen(name, "r");
	CHECK(g != nullptr && fgetc(g) == 'u');
	CHECK(fclose(g) == 0 && fclose(f) == 0);

	/* An invalid mode */
	f = fopen(name, "r");
	CHECK(f != nullptr && setvbuf(f, nullptr, 7, 0) != 0 && fclose(f) == 0);

	/* fflush(NULL) flushes every output stream */
	f = fopen(name, "w");
	CHECK(f != nullptr && fputs("all", f) == 0 && fflush(nullptr) == 0);
	g = fopen(name, "r");
	CHECK(g != nullptr && fgetc(g) == 'a');
	CHECK(fclose(g) == 0 && fclose(f) == 0);

	/* fgets with room for the terminator only */
	f = fopen(name, "r");
	CHECK(f != nullptr);
	strcpy(line, "zzz");
	CHECK(fgets(line, 1, f) == line && line[0] == '\0');
	CHECK(fgets(line, 0, f) == nullptr);
	CHECK(fclose(f) == 0 && remove(name) == 0);
}

static void check_files(void)
{
	char name[L_tmpnam];
	char other[L_tmpnam];
	char *shared;
	char line[16];
	FILE *f;

	/* tmpnam: distinct names, the internal buffer for a null argument */
	temporary(name);
	temporary(other);
	CHECK(strcmp(name, other) != 0);
	shared = tmpnam(nullptr);
	CHECK(shared != nullptr && strcmp(shared, name) != 0 && strlen(shared) < L_tmpnam);

	/* rename and remove */
	f = fopen(name, "w");
	CHECK(f != nullptr && fputs("moved", f) == 0 && fclose(f) == 0);
	CHECK(rename(name, other) == 0);
	errno = 0;
	CHECK(fopen(name, "r") == nullptr && errno == ENOENT);
	f = fopen(other, "r");
	CHECK(f != nullptr && fgets(line, sizeof(line), f) == line && strcmp(line, "moved") == 0 && fclose(f) == 0);
	CHECK(remove(other) == 0);
	errno = 0;
	CHECK(remove(other) != 0 && errno == ENOENT);
	CHECK(rename(name, other) != 0);

	/* tmpfile: opened "wb+", gone when closed */
	f = tmpfile();
	CHECK(f != nullptr);
	CHECK(fputs("temporary", f) == 0 && fseek(f, 0, SEEK_SET) == 0);
	CHECK(fgets(line, sizeof(line), f) == line && strcmp(line, "temporary") == 0);
	CHECK(fclose(f) == 0);

	/* freopen into a new file, then a mode change without a name */
	f = fopen(temporary(name), "w+");
	CHECK(f != nullptr && fputs("first", f) == 0);
	CHECK(freopen(name, "r", f) == f);
	CHECK(fgets(line, sizeof(line), f) == line && strcmp(line, "first") == 0);
	CHECK(freopen(nullptr, "rb", f) == f);
	CHECK(fclose(f) == 0);
	f = fopen(name, "r");
	CHECK(f != nullptr);
	/* Gaining write access needs a reopening, which is not allowed: the stream is closed */
	errno = 0;
	CHECK(freopen(nullptr, "w", f) == nullptr && errno == EBADF);
	CHECK(remove(name) == 0);
}

/* Modes for stdio.run */
static int run_mode(const char *mode)
{
	/* Output without a new-line, flushed by exit after main returns (C23 7.24.4.4p4) */
	if (strcmp(mode, "exit") == 0) {
		printf("partial %d", 7);
		return 3;
	}

	/* perror with a prefix, without one, and an empty one */
	if (strcmp(mode, "perror") == 0) {
		errno = ENOENT;
		perror("open");
		errno = EINVAL;
		perror(nullptr);
		errno = ERANGE;
		perror("");
		return 0;
	}

	/* Lines from stdin, reported on stdout; getchar sees the end */
	if (strcmp(mode, "stdin") == 0) {
		char line[16];
		int lines = 0;
		int characters = 0;

		while (fgets(line, sizeof(line), stdin) != nullptr) {
			++lines;
			characters += (int)strlen(line);
		}
		printf("%d lines %d characters %s\n", lines, characters, feof(stdin) && getchar() == EOF ? "eof" : "error");
		return 0;
	}

	/* A prompt without a new-line must reach stdout before stdin is read (C23 7.23.3p3) */
	if (strcmp(mode, "prompt") == 0) {
		int c;

		fputs("name? ", stdout);
		c = getchar();
		printf("[%c]\n", c);
		return 0;
	}

	/* stderr is unbuffered: it appears before the stdout text that is still buffered */
	if (strcmp(mode, "order") == 0) {
		fputs("out", stdout);
		fputs("err", stderr);
		return 0;
	}

	return 2;
}

int main(int argc, char **argv)
{
	if (argc > 1)
		return run_mode(argv[1]);

	check_write_read();
	check_positioning();
	check_direct();
	check_buffering();
	check_files();

	static_assert(BUFSIZ >= 256 && FOPEN_MAX >= 8 && TMP_MAX >= 25 && L_tmpnam > 0 && FILENAME_MAX > 0);
	static_assert(_IOFBF != _IOLBF && _IOLBF != _IONBF && EOF < 0);
	static_assert(__STDC_VERSION_STDIO_H__ == 202311L);

	if (failures == 0)
		puts("pass: stdio");

	return failures == 0 ? 0 : 1;
}
