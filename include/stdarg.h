/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * stdarg.h
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef __stdarg_h
#define __stdarg_h

/* C23 7.16 */
#define __STDC_VERSION_STDARG_H__ 202311L

typedef __builtin_va_list va_list;

/* C23 7.16.1.4p4: only the first argument is evaluated; the rest is accepted for compatibility */
#define va_start(ap, ...) __builtin_va_start(ap, 0)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#define va_end(ap) __builtin_va_end(ap)

#endif
