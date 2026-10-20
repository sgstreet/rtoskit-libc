/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strcmp.S
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * C23 7.26.4.2 for Thumb-1 (Armv6-M), written for speed (PLAN D75,
 * docs/research/string-speed.md): the byte loop of the size flavour with one
 * index register for both strings, 7 instructions a byte against 8. The
 * characters compare as unsigned char (ldrb zero-extends, 7.26.4p1); the
 * loop stops at the first difference or at a terminator of both, and the
 * result is the difference of the last pair read. A word loop would need a
 * byte search at its exit (Armv6-M has no CLZ) and an entry that the short
 * strings most programs compare would pay for.
 */
	.syntax unified
	.thumb
	.text
	.global strcmp
	.type strcmp, %function
strcmp:
	push {r4, lr}
	movs r4, #0
1:
	ldrb r2, [r0, r4]
	ldrb r3, [r1, r4]
	adds r4, #1
	cmp r2, r3
	bne 2f
	cmp r2, #0
	bne 1b
2:
	subs r0, r2, r3
	pop {r4, pc}
	.size strcmp, . - strcmp

	.section .note.GNU-stack, "", %progbits
