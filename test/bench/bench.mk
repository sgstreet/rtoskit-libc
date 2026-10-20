#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# bench.mk
#
# Created on: Oct 20, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# The phase 3 benchmarks (docs/research/benchmarks.md): built, then run by
# tools/bench-runner ("make bench"); every program links the harness objects

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
SUBDIRS := harness string stdio strto malloc malloc-tiny qsort math threads
string stdio strto malloc qsort math threads: harness
malloc-tiny: malloc
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

include ${PROJECT_ROOT}/tools/makefiles/project.mk

endif
