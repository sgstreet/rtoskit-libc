#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#


# The generic functions, less those the float complex layer of this core provides (PLAN D65)
LIBC_COMPLEX_LAYERS := $(filter sp-nearest,${LIBC_MATH_LAYERS})
LIBC_COMPLEX_OVERRIDES := $(foreach layer,${LIBC_COMPLEX_LAYERS},$(notdir $(wildcard ${SOURCE_DIR}/${layer}/*.c)))
SRC := $(filter-out $(addprefix ${SOURCE_DIR}/,${LIBC_COMPLEX_OVERRIDES}),$(wildcard ${SOURCE_DIR}/*.c))
