#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# size.mk
#
# Created on: Oct 19, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# The phase 2 reference programs (docs/research/size-measurement.md): measured, not run,
# by tools/scripts/size-report

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

include ${PROJECT_ROOT}/tools/makefiles/project.mk

endif
