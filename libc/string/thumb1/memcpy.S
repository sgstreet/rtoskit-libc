/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memcpy.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.2.1 for Thumb-1 (Armv6-M), written for size (PLAN D69): the
 * word-at-a-time algorithm of the C version. When source and destination
 * share the alignment phase, bytes are copied until the destination is
 * aligned, then whole words through ldmia/stmia (Armv6-M faults on unaligned
 * words). The rest is copied forwards with a negative index from the end, so
 * memmove may use this function for any forward copy (dst below src).
 * r12 keeps the return value.
 */
	.syntax unified
	.thumb
	.text
	.global memcpy
	.type memcpy, %function
memcpy:
	mov r12, r0
	movs r3, r0
	eors r3, r1
	lsls r3, r3, #30
	bne 3f
1:
	lsls r3, r0, #30
	beq 2f
	subs r2, #1
	bcc 4f
	ldrb r3, [r1]
	adds r1, #1
	strb r3, [r0]
	adds r0, #1
	b 1b
2:
	subs r2, #4
	bcc 5f
	ldmia r1!, {r3}
	stmia r0!, {r3}
	b 2b
5:
	adds r2, #4
3:
	/* The tail: r0 and r1 to the end, r2 counting up from -n to 0 */
	adds r0, r2
	adds r1, r2
	negs r2, r2
	beq 4f
6:
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	adds r2, #1
	bne 6b
4:
	mov r0, r12
	bx lr
	.size memcpy, . - memcpy

	.section .note.GNU-stack, "", %progbits
