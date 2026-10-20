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
 * C23 7.26.2.3 for Thumb-2 (Armv7-M, Armv7E-M, Armv8-M Mainline), written
 * for speed (PLAN D73, docs/research/string-speed.md). A destination below
 * the source, or past its end ((dst - src) >= n, unsigned), is a forward
 * copy: memcpy, which reads before it writes at rising addresses. Equal
 * pointers and zero bytes return at once. Otherwise the copy runs backwards from the ends,
 * mirroring memcpy: below 32 bytes the tail alone (a byte, a halfword, a
 * word, then 8 and 16 bytes, unaligned accesses allowed, each step skipped by
 * a branch); from 32 bytes the
 * end of the destination is aligned, then 32 bytes per iteration through
 * LDMDB/STMDB, or eight unaligned LDRs and one STMDB when the source end is
 * not aligned, and the tail copies what is left at the start. Every step
 * reads before it writes and moves to lower addresses, which is what makes
 * an overlap with the destination above the source safe.
 */
	.syntax unified
	.thumb
	.text
	.global memmove
	.type memmove, %function
memmove:
	subs r3, r0, r1
	beq 6f
	cmp r3, r2
	bhs memcpy

	mov ip, r0
	add r0, r0, r2
	add r1, r1, r2
	cmp r2, #32
	blo .Ltail
	push {r4-r9, lr}

	/* The end of the destination to a word: (dst + n) & 3, bit 0 a byte, bit 1 a halfword */
	lsls r3, r0, #31
	bpl 1f
	ldrb r3, [r1, #-1]!
	strb r3, [r0, #-1]!
	sub r2, r2, #1
1:
	bcc 2f
	ldrh r3, [r1, #-2]!
	strh r3, [r0, #-2]!
	sub r2, r2, #2
2:

	tst r1, #3
	bne .Lunaligned
	subs r2, r2, #32
	blo .Lrest
3:
	ldmdb r1!, {r3-r9, lr}
	stmdb r0!, {r3-r9, lr}
	subs r2, r2, #32
	bhs 3b
	b .Lrest

.Lunaligned:
	subs r2, r2, #32
	blo .Lrest
4:
	ldr lr, [r1, #-4]
	ldr r9, [r1, #-8]
	ldr r8, [r1, #-12]
	ldr r7, [r1, #-16]
	ldr r6, [r1, #-20]
	ldr r5, [r1, #-24]
	ldr r4, [r1, #-28]
	ldr r3, [r1, #-32]!
	stmdb r0!, {r3-r9, lr}
	subs r2, r2, #32
	bhs 4b

.Lrest:
	pop {r4-r9, lr}

	/* n mod 32 by its bits from the top down (LSLS #31: N is bit 0, C is bit 1) */
.Ltail:
	lsls r3, r2, #31
	beq 5f
	ldrb r3, [r1, #-1]!
	strb r3, [r0, #-1]!
5:
	bcc 7f
	ldrh r3, [r1, #-2]!
	strh r3, [r0, #-2]!
7:
	/* LSLS #29: N is bit 2, C is bit 3; LSLS #28: C is bit 4 */
	lsls r3, r2, #29
	bpl 8f
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
8:
	bcc 10f
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
10:
	lsls r3, r2, #28
	bcc 11f
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
	ldr r3, [r1, #-4]!
	str r3, [r0, #-4]!
11:
	mov r0, ip
6:
	bx lr
	.size memmove, . - memmove

	.section .note.GNU-stack, "", %progbits
