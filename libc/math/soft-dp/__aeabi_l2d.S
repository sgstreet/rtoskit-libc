/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_l2d.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * (double)a, rounded to nearest (C23 6.3.1.4p2, RTABI 4.1.2; PLAN D99): the
 * magnitude, negated in two's complement (INT64_MIN's is 2^63, unsigned),
 * with the sign. Weak, as __aeabi_dadd.
 */
	.syntax unified
	.thumb
	.section .text.__aeabi_l2d,"ax",%progbits
	.weak __aeabi_l2d
	.type __aeabi_l2d, %function
__aeabi_l2d:
	ands r2, r1, #0x80000000
	beq 1f
	negs r0, r0
	sbc r1, r1, r1, lsl #1
1:
	b __libc_softdouble_from_u64
	.size __aeabi_l2d, . - __aeabi_l2d
