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
 * C23 7.26.2.1 for Thumb-1 (Armv6-M), written for speed (PLAN D75,
 * docs/research/string-speed.md). Armv6-M has no unaligned access, so every
 * word access is aligned. When source and destination share their phase,
 * the destination is aligned by bytes, then 16 bytes per iteration go
 * through LDMIA/STMIA of four registers, then words, then bytes, at any
 * size. When the phases differ, below 24 bytes a byte loop with a negative
 * index copies everything; from 24 bytes the destination is aligned by
 * bytes and aligned source words are merged by shifts: with the source k bytes
 * past a boundary, each destination word is (w >> 8k) | (x << (32 - 8k)) of
 * two consecutive source words, two per iteration with the registers
 * alternating (1.6 instructions a byte against 4 for bytes); every source
 * word read holds a byte the copy needs. Every step reads before it writes
 * and moves up, so memmove uses this function for any forward copy.
 */
	.syntax unified
	.thumb
	.text
	.global memcpy
	.type memcpy, %function
memcpy:
	push {r0, r4-r7, lr}
	movs r3, r0
	eors r3, r1
	lsls r3, r3, #30
	bne 12f

	/* Equal phases: bytes to a word boundary, 16-byte blocks, words */
1:
	lsls r3, r0, #30
	beq 2f
	subs r2, #1
	bcc 11f
	ldrb r3, [r1]
	strb r3, [r0]
	adds r1, #1
	adds r0, #1
	b 1b
2:
	subs r2, #16
	blo 4f
3:
	ldmia r1!, {r3-r6}
	stmia r0!, {r3-r6}
	subs r2, #16
	bhs 3b
4:
	adds r2, #12
	blo 6f
5:
	ldmia r1!, {r3}
	stmia r0!, {r3}
	subs r2, #4
	bhs 5b
6:
	adds r2, #4
	b .Lbytes

	/* Different phases: bytes below 24 (where merging does not pay yet), else the destination to a word boundary, then shift and merge */
12:
	cmp r2, #24
	blo .Lbytes
.Lshift:
	lsls r3, r0, #30
	beq 7f
	ldrb r3, [r1]
	strb r3, [r0]
	adds r1, #1
	adds r0, #1
	subs r2, #1
	b .Lshift
7:
	/* r6 = 8k, r7 = 32 - 8k, r1 back to the aligned word holding the next byte */
	lsls r6, r1, #30
	lsrs r6, r6, #27
	movs r7, #32
	subs r7, r7, r6
	lsrs r5, r6, #3
	subs r1, r1, r5
	ldmia r1!, {r3}
	subs r2, #8
	blo 9f
8:
	ldmia r1!, {r4}
	lsrs r3, r6
	movs r5, r4
	lsls r5, r7
	orrs r3, r5
	stmia r0!, {r3}
	ldmia r1!, {r3}
	lsrs r4, r6
	movs r5, r3
	lsls r5, r7
	orrs r4, r5
	stmia r0!, {r4}
	subs r2, #8
	bhs 8b
9:
	/* The source position: k bytes into the word last read */
	adds r2, #8
	lsrs r5, r6, #3
	subs r1, #4
	adds r1, r1, r5

	/* r2 bytes forwards: r0 and r1 to the end, the index counting up from -r2 to 0 */
.Lbytes:
	adds r0, r0, r2
	adds r1, r1, r2
	negs r2, r2
	beq 11f
10:
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	adds r2, #1
	bne 10b
11:
	pop {r0, r4-r7, pc}
	.size memcpy, . - memcpy

	.section .note.GNU-stack, "", %progbits
