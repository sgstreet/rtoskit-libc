/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memset.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.6.1 for Thumb-1 (Armv6-M), written for size (PLAN D69): the
 * byte replicated into a word, bytes until the destination is aligned, whole
 * words through stmia, then the remaining bytes with an index counting down.
 * r12 keeps the return value.
 */
	.syntax unified
	.thumb
	.text
	.global memset
	.type memset, %function
memset:
	mov r12, r0
	uxtb r1, r1
	lsls r3, r1, #8
	orrs r1, r3
	lsls r3, r1, #16
	orrs r1, r3
1:
	lsls r3, r0, #30
	beq 2f
	subs r2, #1
	bcc 4f
	strb r1, [r0]
	adds r0, #1
	b 1b
2:
	subs r2, #4
	bcc 3f
	stmia r0!, {r1}
	b 2b
3:
	adds r2, #3
	bmi 4f
5:
	strb r1, [r0, r2]
	subs r2, #1
	bpl 5b
4:
	mov r0, r12
	bx lr
	.size memset, . - memset

	.section .note.GNU-stack, "", %progbits
