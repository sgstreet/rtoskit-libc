/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * generic.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * The generic sources of libc/math under other names, so the test can call
 * them beside the library's FPU-layer versions (D46).
 */
#define fabs generic_fabs
#define fabsf generic_fabsf
#define fma generic_fma
#define fmaf generic_fmaf
#define rint generic_rint
#define rintf generic_rintf
#define nearbyint generic_nearbyint
#define nearbyintf generic_nearbyintf
#define trunc generic_trunc
#define truncf generic_truncf
#define floor generic_floor
#define floorf generic_floorf
#define ceil generic_ceil
#define ceilf generic_ceilf
#define round generic_round
#define roundf generic_roundf
#define roundeven generic_roundeven
#define roundevenf generic_roundevenf
#define fmax generic_fmax
#define fmaxf generic_fmaxf
#define fmin generic_fmin
#define fminf generic_fminf

#include <math.h>

#include "math/fabs.c"
#include "math/fabsf.c"
#include "math/fma.c"
#include "math/fmaf.c"
#include "math/rint.c"
#include "math/rintf.c"
#include "math/nearbyint.c"
#include "math/nearbyintf.c"
#include "math/trunc.c"
#include "math/truncf.c"
#include "math/floor.c"
#include "math/floorf.c"
#include "math/ceil.c"
#include "math/ceilf.c"
#include "math/round.c"
#include "math/roundf.c"
#include "math/roundeven.c"
#include "math/roundevenf.c"
#include "math/fmax.c"
#include "math/fmaxf.c"
#include "math/fmin.c"
#include "math/fminf.c"
