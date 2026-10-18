/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdckdint.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _STDCKDINT_H
#define _STDCKDINT_H

/* C23 7.20 */
#define __STDC_VERSION_STDCKDINT_H__ 202311L

/*
 * C23 7.20.1: the builtins compute in infinite precision, store the result
 * wrapped to the type of *result and return true when it did not fit.
 */
#define ckd_add(result, a, b) __builtin_add_overflow((a), (b), (result))
#define ckd_sub(result, a, b) __builtin_sub_overflow((a), (b), (result))
#define ckd_mul(result, a, b) __builtin_mul_overflow((a), (b), (result))

#endif /* _STDCKDINT_H */
