/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * linux-arch.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LINUX_ARCH_H
#define _LINUX_ARCH_H

#include <stdint.h>

/*
 * x86_64 Linux system call interface: the number goes in rax, arguments in
 * rdi, rsi, rdx, r10, r8 and r9, the result comes back in rax, and the
 * kernel clobbers rcx and r11. Errors are returned as -4095..-1.
 */

#define __LINUX_SYS_read 0
#define __LINUX_SYS_write 1
#define __LINUX_SYS_mmap 9
#define __LINUX_SYS_getpid 39
#define __LINUX_SYS_arch_prctl 158
#define __LINUX_SYS_gettid 186
#define __LINUX_SYS_exit_group 231
#define __LINUX_SYS_tgkill 234

#define __LINUX_ARCH_SET_FS 0x1002

static inline long __linux_syscall0(long number)
{
	long result;
	__asm__ volatile("syscall" : "=a"(result) : "a"(number) : "rcx", "r11", "memory");
	return result;
}

static inline long __linux_syscall1(long number, long a)
{
	long result;
	__asm__ volatile("syscall" : "=a"(result) : "a"(number), "D"(a) : "rcx", "r11", "memory");
	return result;
}

static inline long __linux_syscall2(long number, long a, long b)
{
	long result;
	__asm__ volatile("syscall" : "=a"(result) : "a"(number), "D"(a), "S"(b) : "rcx", "r11", "memory");
	return result;
}

static inline long __linux_syscall3(long number, long a, long b, long c)
{
	long result;
	__asm__ volatile("syscall" : "=a"(result) : "a"(number), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
	return result;
}

static inline long __linux_syscall6(long number, long a, long b, long c, long d, long e, long f)
{
	long result;
	register long r10 __asm__("r10") = d;
	register long r8 __asm__("r8") = e;
	register long r9 __asm__("r9") = f;
	__asm__ volatile("syscall" : "=a"(result) : "a"(number), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
	return result;
}

/* ELF64 file and program headers, only the fields startup reads are meaningful */
struct __linux_ehdr {
	unsigned char e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct __linux_phdr {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

#endif
