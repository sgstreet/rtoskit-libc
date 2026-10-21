#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: Oct 21, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# The helpers are called directly only where they use this build's calling convention: no FPU, or x86_64 (PLAN D85)
SRC := ${SOURCE_DIR}/softfp.c ${SOURCE_DIR}/$(if $(filter nofp sse2,${LIBC_FPU}),float_operations.c,float_operations_fpu.c)
