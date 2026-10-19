/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __rtos_linux_syscall.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * long __rtos_linux_syscall(long a, long b, long c, long d, long e, long f, long number)
 *
 * Arm EABI Linux system call: the number goes in r7, the arguments in r0-r5,
 * and the result comes back in r0. AAPCS passes a-d in r0-r3 and e, f and
 * the number on the stack. Out of line because r7 is the Thumb frame pointer,
 * which inline asm may not claim in unoptimised code. Thumb-1 only.
 *
 * "svc #0" is emitted as raw bits: plain Armv6-M has no SVC (only Armv6S-M
 * does) and the assembler rejects it there, but this port always runs as a
 * Linux process, where the instruction exists (PLAN D37).
 */
	.syntax unified
	.thumb
	.text
	.global __rtos_linux_syscall
	.type __rtos_linux_syscall, %function
__rtos_linux_syscall:
	push {r4, r5, r7, lr}
	ldr r4, [sp, #16]
	ldr r5, [sp, #20]
	ldr r7, [sp, #24]
	.inst.n 0xdf00
	pop {r4, r5, r7, pc}
	.size __rtos_linux_syscall, . - __rtos_linux_syscall

	.section .note.GNU-stack, "", %progbits
