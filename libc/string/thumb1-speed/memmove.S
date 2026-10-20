/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * memmove.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.2.3 for Thumb-1 (Armv6-M), written for speed (PLAN D75,
 * docs/research/string-speed.md). A destination below the source, or past
 * its end ((dst - src) >= n, unsigned), is a forward copy, which memcpy does
 * strictly upwards (a tail call through a register: a Thumb-1 B reaches only
 * 2 KB); equal pointers return at once. Otherwise the copy runs down, the
 * count of bytes left serving as the index from the starts: for equal
 * phases the end of the destination is aligned by bytes (at most 3: an
 * overlap with equal phases puts the destination 4 or more bytes up, and n
 * above that), then 16 bytes per iteration through LDMIA/STMIA of four
 * registers at start + index, then words, then bytes, at any size; for
 * different phases bytes below 24, else the end of the destination is
 * aligned and aligned source words are merged by shifts as in memcpy,
 * going down. Every step reads before it
 * writes and moves down, which makes an overlap with the destination above
 * the source safe.
 */
	.syntax unified
	.thumb
	.text
	.global memmove
	.type memmove, %function
memmove:
	subs r3, r0, r1
	beq 1f
	cmp r3, r2
	blo 2f
	ldr r3, =memcpy
	bx r3
1:
	bx lr

2:
	push {r0, r4-r7, lr}
	lsls r3, r3, #30
	bne 12f
	/* Equal phases: the end of the destination to a word boundary */
3:
	adds r3, r0, r2
	lsls r3, r3, #30
	beq 4f
	subs r2, #1
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	b 3b
4:
	subs r2, #16
	blo 6f
5:
	adds r5, r1, r2
	ldmia r5!, {r3, r4, r6, r7}
	adds r5, r0, r2
	stmia r5!, {r3, r4, r6, r7}
	subs r2, #16
	bhs 5b
6:
	adds r2, #12
	blo 8f
7:
	ldr r3, [r1, r2]
	str r3, [r0, r2]
	subs r2, #4
	bhs 7b
8:
	adds r2, #4

	/* r2 bytes down from index r2 - 1 to 0 */
.Lbytes:
	cmp r2, #0
	beq 11f
10:
	subs r2, #1
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	bne 10b
11:
	pop {r0, r4-r7, pc}

	/*
	 * Different phases: bytes below 24 (where merging does not pay yet).
	 * Else, from the ends: with the source end k bytes past a boundary, the
	 * word ending at the destination end is (p >> 8k) | (w << (32 - 8k)) of
	 * the aligned source word w holding those k bytes and the word p below it
	 */
12:
	cmp r2, #24
	blo .Lbytes
13:
	adds r3, r0, r2
	lsls r3, r3, #30
	beq 14f
	subs r2, #1
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	b 13b
14:
	adds r0, r0, r2
	adds r1, r1, r2
	lsls r6, r1, #30
	lsrs r6, r6, #27
	movs r7, #32
	subs r7, r7, r6
	lsrs r5, r6, #3
	subs r1, r1, r5
	ldr r3, [r1]
	subs r2, #8
	blo 16f
15:
	subs r1, #4
	ldr r4, [r1]
	lsls r3, r7
	movs r5, r4
	lsrs r5, r6
	orrs r3, r5
	subs r0, #4
	str r3, [r0]
	subs r1, #4
	ldr r3, [r1]
	lsls r4, r7
	movs r5, r3
	lsrs r5, r6
	orrs r4, r5
	subs r0, #4
	str r4, [r0]
	subs r2, #8
	bhs 15b
16:
	/* The rest: r2 bytes below the source end (k bytes into the word last read) and the destination end */
	adds r2, #8
	lsrs r5, r6, #3
	adds r1, r1, r5
	subs r0, r0, r2
	subs r1, r1, r2
	b .Lbytes
	.ltorg
	.size memmove, . - memmove

	.section .note.GNU-stack, "", %progbits
