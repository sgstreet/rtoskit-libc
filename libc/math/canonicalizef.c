/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * canonicalizef.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

/*
 * C23 7.12.11.7: every binary encoding is canonical; a signaling NaN
 * becomes quiet, raising invalid. Always succeeds.
 */
int canonicalizef(float *cx, const float *x)
{
	float value = *x;

	*cx = issignaling(value) ? value + value : value;
	return 0;
}
