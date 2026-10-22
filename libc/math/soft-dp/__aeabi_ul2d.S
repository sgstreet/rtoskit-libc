/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __aeabi_ul2d.S
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/* (double)a, rounded to nearest (C23 6.3.1.4p2, RTABI 4.1.2; PLAN D99). Weak, as __aeabi_dadd. */
	.syntax unified
	.thumb
	.section .text.__aeabi_ul2d,"ax",%progbits
	.weak __aeabi_ul2d
	.type __aeabi_ul2d, %function
__aeabi_ul2d:
	movs r2, #0
	b __libc_softdouble_from_u64
	.size __aeabi_ul2d, . - __aeabi_ul2d
