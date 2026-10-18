#!/bin/bash
#
# Copyright (C) 2026 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# session-start.sh
#
# Created on: Oct 18, 2026
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

# Cloud session setup: the toolchain for `make` and `make check` (docs/toolchain.md)
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
	exit 0
fi

packages="gcc-14 qemu-user clang-format"
missing=""
for package in ${packages}; do
	dpkg -s "${package}" > /dev/null 2>&1 || missing="${missing} ${package}"
done

if [ -n "${missing}" ]; then
	export DEBIAN_FRONTEND=noninteractive
	apt-get update -q
	apt-get install -y -q ${missing}
fi

cd "${CLAUDE_PROJECT_DIR:-$(dirname "$0")/../..}"
git submodule update --init third-party/libc-test
