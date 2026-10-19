/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * longjmp.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.13.2.1 for Armv6-M: restore what setjmp saved and return from that
 * setjmp again, with val, or 1 when val is 0 (p4). The high registers are
 * restored first, through r2-r4, before r4-r7 themselves.
 */
	.syntax unified
	.thumb
	.text
	.global longjmp
	.type longjmp, %function
longjmp:
	adds r0, #16
	ldmia r0!, {r2-r4}
	mov r8, r2
	mov r9, r3
	mov r10, r4
	ldmia r0!, {r2-r4}
	mov r11, r2
	mov sp, r3
	mov lr, r4
	subs r0, #40
	ldmia r0!, {r4-r7}
	movs r0, r1
	bne 1f
	movs r0, #1
1:
	bx lr
	.size longjmp, . - longjmp

	.section .note.GNU-stack, "", %progbits
