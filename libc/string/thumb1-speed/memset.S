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
 * C23 7.26.6.1 for Thumb-1 (Armv6-M), written for speed (PLAN D75,
 * docs/research/string-speed.md): the byte replicated into a word; the
 * destination aligned by bytes; from 32 bytes two STMIAs of four registers
 * per iteration (the only path that saves registers), then 16; then words,
 * then bytes. r12 keeps the return value.
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
	bcc 9f
	strb r1, [r0]
	adds r0, #1
	b 1b
2:
	cmp r2, #32
	blo 5f
	push {r4, r5}
	movs r3, r1
	movs r4, r1
	movs r5, r1
	subs r2, #32
3:
	stmia r0!, {r1, r3, r4, r5}
	stmia r0!, {r1, r3, r4, r5}
	subs r2, #32
	bhs 3b
	/* n mod 32: 16 once */
	adds r2, #32
	cmp r2, #16
	blo 4f
	stmia r0!, {r1, r3, r4, r5}
	subs r2, #16
4:
	pop {r4, r5}
	/* Words, then bytes from the last down */
5:
	subs r2, #4
	bcc 8f
7:
	stmia r0!, {r1}
	subs r2, #4
	bhs 7b
8:
	adds r2, #3
	bmi 9f
6:
	strb r1, [r0, r2]
	subs r2, #1
	bpl 6b
9:
	mov r0, r12
	bx lr
	.size memset, . - memset

	.section .note.GNU-stack, "", %progbits
