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
 * C23 7.26.2.3 for Thumb-1 (Armv6-M), written for size (PLAN D69): a
 * destination below the source, or past its end ((dst - src) >= n,
 * unsigned), is a forward copy, which memcpy does strictly forwards; the
 * overlapping case copies backwards with indexed loads and stores: bytes
 * until the end of the destination is aligned, then words when both share
 * the alignment phase, then the remaining bytes. Equal pointers return at
 * once.
 */
	.syntax unified
	.thumb
	.text
	.global memmove
	.type memmove, %function
memmove:
	subs r3, r0, r1
	beq 4f
	cmp r3, r2
	bhs 6f
	lsls r3, r3, #30
	bne 3f
1:
	/* Equal phases with distinct pointers put the destination at least 4 bytes past the source and n above that, so at most 3 bytes go before the end is aligned */
	adds r3, r0, r2
	lsls r3, r3, #30
	beq 2f
	subs r2, #1
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	b 1b
2:
	subs r2, #4
	bcc 5f
	ldr r3, [r1, r2]
	str r3, [r0, r2]
	b 2b
5:
	adds r2, #4
3:
	subs r2, #1
	bcc 4f
	ldrb r3, [r1, r2]
	strb r3, [r0, r2]
	b 3b
4:
	bx lr
6:
	/* A Thumb-1 b reaches only 2 KB, so the tail call goes through a register */
	ldr r3, =memcpy
	bx r3
	.ltorg
	.size memmove, . - memmove

	.section .note.GNU-stack, "", %progbits
