/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * setjmp.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.13.1.1 for Armv6S-M (AAPCS 5.1.1, no FPU): save the callee-saved
 * r4-r11, the stack pointer and the return address. Thumb-1 can only store
 * r0-r7, so the high registers go through low ones.
 * jmp_buf layout: r4-r11, sp, lr.
 */
	.syntax unified
	.thumb
	.text
	.global setjmp
	.type setjmp, %function
setjmp:
	stmia r0!, {r4-r7}
	mov r1, r8
	mov r2, r9
	mov r3, r10
	stmia r0!, {r1-r3}
	mov r1, r11
	mov r2, sp
	mov r3, lr
	stmia r0!, {r1-r3}
	movs r0, #0
	bx lr
	.size setjmp, . - setjmp

	.section .note.GNU-stack, "", %progbits
