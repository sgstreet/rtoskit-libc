/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * fminimum_mag_numf.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* C23 7.12.12.11: the number of smaller magnitude, NaNs as in fminimum_num */
float fminimum_mag_numf(float x, float y)
{
	if (isnan(x) || isnan(y))
		return fminimum_numf(x, y);

	return fminimum_magf(x, y);
}
