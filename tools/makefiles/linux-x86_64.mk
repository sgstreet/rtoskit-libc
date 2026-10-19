#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# linux-x86_64.mk
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Linux process on x86_64: the native libc-test host, bound to port/linux

export ARCH_CROSS := x86_64
export RTOS_PORT := linux

# The allocator built into libc.a: nano (small, the default) or tlsf (constant time); PLAN D32
export LIBC_MALLOC ?= nano
