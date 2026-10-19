/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdio.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _STDIO_H
#define _STDIO_H

/* C23 7.23 (the scanf family arrives with P1.10) */
#define __STDC_VERSION_STDIO_H__ 202311L

typedef __SIZE_TYPE__ size_t;

typedef struct __libc_file FILE;

/* A position of a byte stream; there are no multibyte parse states to keep */
typedef struct {
	long long __offset;
} fpos_t;

#define NULL ((void *)0)

/* setvbuf modes (7.23.5.6) */
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

/* The default buffer size, the C minimum: RAM per stream matters more than speed (docs/research/stdio.md) */
#define BUFSIZ 256

#define EOF (-1)

#define FOPEN_MAX 16
#define FILENAME_MAX 256
#define L_tmpnam 32
#define TMP_MAX 10000

/* printf writes NaN as "nan" without an n-char-sequence (7.23.1p3) */
#define _PRINTF_NAN_LEN_MAX 0

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern FILE __libc_stdin;
extern FILE __libc_stdout;
extern FILE __libc_stderr;

#define stdin (&__libc_stdin)
#define stdout (&__libc_stdout)
#define stderr (&__libc_stderr)

/* Operations on files (7.23.4) */
int remove(const char *filename);
int rename(const char *old, const char *new);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* File access functions (7.23.5) */
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char *restrict filename, const char *restrict mode);
FILE *freopen(const char *restrict filename, const char *restrict mode, FILE *restrict stream);
void setbuf(FILE *restrict stream, char *restrict buf);
int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size);

/* Formatted input/output functions (7.23.6) */
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int printf(const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
int sprintf(char *restrict s, const char *restrict format, ...);
int vfprintf(FILE *restrict stream, const char *restrict format, __builtin_va_list arg);
int vprintf(const char *restrict format, __builtin_va_list arg);
int vsnprintf(char *restrict s, size_t n, const char *restrict format, __builtin_va_list arg);
int vsprintf(char *restrict s, const char *restrict format, __builtin_va_list arg);

/* Character input/output functions (7.23.7) */
int fgetc(FILE *stream);
char *fgets(char *restrict s, int n, FILE *restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char *restrict s, FILE *restrict stream);
int getc(FILE *stream);
int getchar(void);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);

/* Direct input/output functions (7.23.8) */
size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);

/* File positioning functions (7.23.9) */
int fgetpos(FILE *restrict stream, fpos_t *restrict pos);
int fseek(FILE *stream, long int offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long int ftell(FILE *stream);
void rewind(FILE *stream);

/* Error-handling functions (7.23.10) */
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

#endif /* _STDIO_H */
