/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __powidf2.c
 *
 * Created on: Oct 22, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>

#include "internal/softdouble.h"

/*
 * x^n for an int n, the compiler's helper for __builtin_powi (PLAN D100):
 * square and multiply over n's bits, and the reciprocal for a negative n.
 * Like libgcc's, it rounds at every step (it is not pown); libgcc's would
 * bring its own double helpers with it.
 */
double __powidf2(double x, int n)
{
	unsigned int bits = n < 0 ? 0u - (unsigned int)n : (unsigned int)n;
	double result = (bits & 1) != 0 ? x : 1.0;

	while ((bits >>= 1) != 0) {
		x = x * x;
		if ((bits & 1) != 0)
			result = result * x;
	}

	return n < 0 ? 1.0 / result : result;
}
