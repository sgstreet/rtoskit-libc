/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * smoke.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <rtos/rtos-port.h>

/*
 * Bootstrap smoke test for the Linux port (P1.01): thread-local storage is
 * initialised from the TLS image, errno is per TLS block, arguments reach
 * main and the exit status reaches the parent (see smoke.run).
 */

/* Linux port services used to build and switch to a second TLS block */
extern struct __libc_tls_image __rtos_linux_tls_image;
void __rtos_linux_set_tp(void *tp);
void *__rtos_linux_get_tp(void);

static thread_local int tdata_value = 42;
static thread_local char tbss_buffer[64];
static thread_local alignas(64) char aligned_value = 'a';

static int failures;

/* Not inlined or analysed so every access recomputes the address from the current thread pointer */
[[gnu::noipa]] static int get_errno(void)
{
	return errno;
}

[[gnu::noipa]] static void set_errno(int value)
{
	errno = value;
}

[[gnu::noipa]] static int *errno_address(void)
{
	return &errno;
}

[[gnu::noipa]] static int get_tdata(void)
{
	return tdata_value;
}

[[gnu::noipa]] static void set_tdata(int value)
{
	tdata_value = value;
}

[[gnu::noipa]] static bool aligned_ok(void)
{
	return ((uintptr_t)&aligned_value & 63) == 0 && aligned_value == 'a';
}

[[gnu::noipa]] static bool tbss_is_zero(void)
{
	for (size_t i = 0; i < sizeof(tbss_buffer); ++i)
		if (tbss_buffer[i] != 0)
			return false;
	return true;
}

static void check(bool ok, const char *what)
{
	fputs(ok ? "PASS: " : "FAIL: ", stdout);
	puts(what);
	if (!ok)
		++failures;
}

static bool equals(const char *a, const char *b)
{
	while (*a && *a == *b) {
		++a;
		++b;
	}
	return *a == *b;
}

static int to_int(const char *s)
{
	int value = 0;
	while (*s >= '0' && *s <= '9')
		value = value * 10 + (*s++ - '0');
	return value;
}

static void check_initial_thread(void)
{
	check(get_errno() == 0, "errno is zero at startup");
	check(get_tdata() == 42, ".tdata initialised from the TLS image");
	check(tbss_is_zero(), ".tbss zero filled");
	check(aligned_ok(), "over-aligned TLS variable aligned and initialised");
}

static void check_second_block(void)
{
	static unsigned char block[4096];

	size_t size = __libc_tls_block_size(&__rtos_linux_tls_image);
	check(size <= sizeof(block), "TLS block fits the test buffer");
	if (size > sizeof(block))
		return;

	set_errno(5);
	set_tdata(17);
	int *main_errno = errno_address();
	void *main_tp = __rtos_linux_get_tp();

	void *tp = __libc_tls_init(block, &__rtos_linux_tls_image);
	check((unsigned char *)tp >= block && (unsigned char *)tp < block + sizeof(block), "thread pointer inside the block");
	__rtos_linux_set_tp(tp);

	check(__rtos_linux_get_tp() == tp, "control block points to itself");
	check(errno_address() != main_errno, "errno has a distinct address per block");
	check(get_errno() == 0, "errno starts at zero in a new block");
	check(get_tdata() == 42, ".tdata initialised in a new block");
	check(tbss_is_zero(), ".tbss zero filled in a new block");
	check(aligned_ok(), "over-aligned TLS variable aligned in a new block");
	set_errno(9);
	set_tdata(99);

	__rtos_linux_set_tp(main_tp);

	check(get_errno() == 5, "errno unchanged in the initial block");
	check(get_tdata() == 17, ".tdata unchanged in the initial block");
	set_errno(0);
}

int main(int argc, char **argv)
{
	/* "smoke exit <status>" exits with that status for smoke.run to observe */
	if (argc == 3 && equals(argv[1], "exit"))
		exit(to_int(argv[2]));

	check(argc >= 1 && argv[argc] == nullptr, "argv is null terminated");
	check_initial_thread();
	check_second_block();

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
