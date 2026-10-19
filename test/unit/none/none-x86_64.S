/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * none-x86_64.S
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */
	.text
	.global _start
	.type _start, @function
_start:
	xor %ebp, %ebp
	mov %rsp, %rdi
	and $-16, %rsp
	call none_test_start
	hlt
	.size _start, . - _start

	/* void none_test_write(const void *buffer, size_t count): write(1, buffer, count) */
	.global none_test_write
	.type none_test_write, @function
none_test_write:
	mov %rsi, %rdx
	mov %rdi, %rsi
	mov $1, %edi
	mov $1, %eax
	syscall
	ret
	.size none_test_write, . - none_test_write

	/* void none_test_exit(int status): exit_group(status) */
	.global none_test_exit
	.type none_test_exit, @function
none_test_exit:
	mov $231, %eax
	syscall
	hlt
	.size none_test_exit, . - none_test_exit

	/*
	 * void __rtos_none_set_tp(void *tp): replaces the port's wrfsbase, which
	 * a process may not use without kernel support, by arch_prctl(ARCH_SET_FS, tp)
	 */
	.global __rtos_none_set_tp
	.type __rtos_none_set_tp, @function
__rtos_none_set_tp:
	mov %rdi, %rsi
	mov $0x1002, %edi
	mov $158, %eax
	syscall
	ret
	.size __rtos_none_set_tp, . - __rtos_none_set_tp

	.section .note.GNU-stack, "", @progbits
