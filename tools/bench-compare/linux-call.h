/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * linux-call.h
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LINUX_CALL_H
#define _LINUX_CALL_H

/*
 * A Linux system call from Thumb code: the number in r7, the arguments in
 * r0 .. r2, the result in r0. "svc #0" as raw bits, since plain Armv6-M
 * has no SVC (it is qemu-arm's Linux process that runs it, as in
 * port/linux/arm, PLAN D37).
 */
static inline long linux_call(long number, long a, long b, long c)
{
	register long r0 __asm__("r0") = a;
	register long r1 __asm__("r1") = b;
	register long r2 __asm__("r2") = c;
	register long r7 __asm__("r7") = number;

	__asm__ volatile(".inst.n 0xdf00" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory");
	return r0;
}

#define LINUX_READ 3
#define LINUX_WRITE 4
#define LINUX_OPEN 5
#define LINUX_CLOSE 6
#define LINUX_LSEEK 19
#define LINUX_EXIT_GROUP 248

/* The heap the comparison libraries' allocators grow into (the benchmarks need a few hundred KiB) */
#define HEAP_SIZE (4 << 20)

#endif /* _LINUX_CALL_H */
