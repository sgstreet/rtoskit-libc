/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * layer_none.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include "softdp.h"

/* No soft-dp layer to check here: it is compiled into the test on x86_64 and linked on armv7e-m and armv8-m.main (PLAN D99) */
void test_layer([[maybe_unused]] unsigned long iterations, [[maybe_unused]] unsigned long float_stride)
{
}
