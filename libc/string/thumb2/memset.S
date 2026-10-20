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
 * C23 7.26.6.1 for Thumb-2 (Armv7-M, Armv7E-M, Armv8-M Mainline), written
 * for speed (PLAN D73, docs/research/string-speed.md): zero bytes return at
 * once; the byte is replicated into a word (times 0x01010101); below 32 bytes the tail alone
 * stores n by its bits (four words, two, one, a halfword, a byte, unaligned
 * stores allowed, each step skipped by a branch); from 32 bytes one
 * unaligned word is stored and the destination advances to the next word
 * boundary (the bytes stored twice hold the same value), 32 bytes per iteration
 * go out through two STMs of four registers, then 16 through one, and the
 * tail stores the rest. ip keeps the return value.
 */
	.syntax unified
	.thumb
	.text
	.global memset
	.type memset, %function
memset:
	cbz r2, 9f
	mov ip, r0
	uxtb r1, r1
	mov r3, #0x01010101
	muls r1, r3, r1
	cmp r2, #32
	blo .Ltail

	/* One unaligned word at dst, then dst advances by k = (-dst) & 3 to a word boundary */
	str r1, [r0]
	negs r3, r0
	lsls r3, r3, #30
	lsrs r3, r3, #30
	subs r2, r2, r3
	add r0, r0, r3

	push {r4, r5}
	mov r3, r1
	mov r4, r1
	mov r5, r1
	subs r2, r2, #32
	blo 2f
1:
	stmia r0!, {r1, r3, r4, r5}
	stmia r0!, {r1, r3, r4, r5}
	subs r2, r2, #32
	bhs 1b
2:
	tst r2, #16
	it ne
	stmiane r0!, {r1, r3, r4, r5}
	pop {r4, r5}

	/* n mod 16 by its bits (LSLS #29: C is bit 3, N is bit 2) */
	b 5f

	/* Below 32 bytes: 16 as four words first (LSLS #28: C is bit 4) */
.Ltail:
	lsls r3, r2, #28
	bcc 5f
	str r1, [r0]
	str r1, [r0, #4]
	str r1, [r0, #8]
	str r1, [r0, #12]
	adds r0, r0, #16
5:
	lsls r3, r2, #29
	bcc 6f
	str r1, [r0]
	str r1, [r0, #4]
	adds r0, r0, #8
6:
	lsls r3, r2, #30
	bcc 7f
	str r1, [r0], #4
7:
	/* LSLS #31: C is bit 1; Z is clear for bit 0 */
	lsls r3, r2, #31
	bcc 8f
	strh r1, [r0], #2
8:
	beq 10f
	strb r1, [r0]
10:
	mov r0, ip
9:
	bx lr
	.size memset, . - memset

	.section .note.GNU-stack, "", %progbits
