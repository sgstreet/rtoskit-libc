/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * math.h
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _MATH_H
#define _MATH_H

/*
 * C23 7.12: for now only the constants of p3-p5, which strtod's contract
 * (7.24.1.5p10) uses; the types, classification macros and functions come
 * with libm (P1.13 onwards). INFINITY and NAN repeat <float.h>'s identical
 * definitions (C23 keeps them in both headers).
 */

#define HUGE_VAL (__builtin_huge_val())
#define HUGE_VALF (__builtin_huge_valf())
#define HUGE_VALL (__builtin_huge_vall())

#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))

#endif /* _MATH_H */
