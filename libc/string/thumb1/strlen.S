/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strlen.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.6.4 for Thumb-1 (Armv6-M), written for size (PLAN D69): bytes
 * until the pointer is aligned, then whole words until one has a zero byte
 * ((w - 0x01010101) & ~w & 0x80808080, exact for the first zero byte), then
 * bytes again from the start of that word. r12 keeps the start.
 */
	.syntax unified
	.thumb
	.text
	.global strlen
	.type strlen, %function
strlen:
	push {r4, lr}
	mov r12, r0
	ldr r3, =0x01010101
	lsls r4, r3, #7
1:
	lsls r2, r0, #30
	beq 2f
3:
	ldrb r2, [r0]
	cmp r2, #0
	beq 4f
	adds r0, #1
	b 1b
2:
	ldmia r0!, {r1}
	subs r2, r1, r3
	bics r2, r1
	tst r2, r4
	beq 2b
	subs r0, #4
	/* The word holds a zero byte: the byte loop finds it before the pointer is aligned again */
	b 3b
4:
	mov r1, r12
	subs r0, r0, r1
	pop {r4, pc}
	.ltorg
	.size strlen, . - strlen

	.section .note.GNU-stack, "", %progbits
