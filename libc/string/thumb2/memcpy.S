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
 * C23 7.26.2.1 for Thumb-2 (Armv7-M, Armv7E-M, Armv8-M Mainline), written
 * for speed (PLAN D73, docs/research/string-speed.md). These cores allow
 * unaligned LDR/STR/LDRH/STRH (not LDM/STM/LDRD/STRD), as GCC also assumes
 * for them (-munaligned-access, __ARM_FEATURE_UNALIGNED).
 *
 * Zero bytes return at once. Below 32 bytes the tail alone copies n by its
 * bits: 16 and 8 as words, then a word, a halfword and a byte, at any
 * alignment; each step a branch skips, so an absent one costs one
 * instruction (the 16 and 8 byte steps use offsets and one pointer update,
 * the short encodings, for the size budget). From 32 bytes the destination is aligned first (a byte,
 * then a halfword); then 32 bytes per iteration go through LDM/STM of eight
 * registers when the source is aligned too, or through eight unaligned
 * LDRs and one STM when it is not, and the tail copies the rest. Every
 * step reads before it writes and moves to higher addresses, so memmove
 * uses this function for any forward copy (dst below src). The flags of
 * the remainder are those of n - 32 (the low five bits are the same).
 * ip keeps the return value.
 */
	.syntax unified
	.thumb
	.text
	.global memcpy
	.type memcpy, %function
memcpy:
	cbnz r2, 1f
	bx lr
1:
	mov ip, r0
	cmp r2, #32
	blo .Ltail
	push {r4-r9, lr}

	/* k = (-dst) & 3 leaves n; then bit 0 of k a byte, bit 1 a halfword (LSLS #31: N is bit 0, C is bit 1) */
	negs r3, r0
	lsls r3, r3, #30
	lsrs r3, r3, #30
	subs r2, r2, r3
	lsls r3, r3, #31
	bpl 11f
	ldrb r3, [r1], #1
	strb r3, [r0], #1
11:
	bcc 2f
	ldrh r3, [r1], #2
	strh r3, [r0], #2
2:

	/* Source aligned too (LSLS #30: Z when bits 1:0 are clear) */
	lsls r3, r1, #30
	bne .Lunaligned
	subs r2, r2, #32
	blo .Lrest
3:
	ldmia r1!, {r3-r9, lr}
	stmia r0!, {r3-r9, lr}
	subs r2, r2, #32
	bhs 3b
	b .Lrest

.Lunaligned:
	subs r2, r2, #32
	blo .Lrest
4:
	ldr r3, [r1]
	ldr r4, [r1, #4]
	ldr r5, [r1, #8]
	ldr r6, [r1, #12]
	ldr r7, [r1, #16]
	ldr r8, [r1, #20]
	ldr r9, [r1, #24]
	ldr lr, [r1, #28]
	adds r1, r1, #32
	stmia r0!, {r3-r9, lr}
	subs r2, r2, #32
	bhs 4b

.Lrest:
	pop {r4-r9, lr}

	/* n mod 32 by its bits (LSLS #28: C is bit 4, N is bit 3) */
.Ltail:
	lsls r3, r2, #28
	bcc 5f
	ldr r3, [r1]
	str r3, [r0]
	ldr r3, [r1, #4]
	str r3, [r0, #4]
	ldr r3, [r1, #8]
	str r3, [r0, #8]
	ldr r3, [r1, #12]
	str r3, [r0, #12]
	adds r0, r0, #16
	adds r1, r1, #16
5:
	/* LSLS #29: C is bit 3 */
	lsls r3, r2, #29
	bcc 6f
	ldr r3, [r1]
	str r3, [r0]
	ldr r3, [r1, #4]
	str r3, [r0, #4]
	adds r0, r0, #8
	adds r1, r1, #8
6:
	/* LSLS #30: C is bit 2, N is bit 1; LSLS #31: Z is clear for bit 0 */
	lsls r3, r2, #30
	bcc 7f
	ldr r3, [r1], #4
	str r3, [r0], #4
7:
	bpl 8f
	ldrh r3, [r1], #2
	strh r3, [r0], #2
8:
	lsls r3, r2, #31
	beq 10f
	ldrb r3, [r1]
	strb r3, [r0]
10:
	mov r0, ip
	bx lr
	.size memcpy, . - memcpy

	.section .note.GNU-stack, "", %progbits
