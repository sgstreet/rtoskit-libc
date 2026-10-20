/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * close-trace.h
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _CLOSE_TRACE_H
#define _CLOSE_TRACE_H

/* stdio.mk links with --wrap=__rtos_close: the library's closes reach the wrapper first */
int __real___rtos_close(int handle);
int __wrap___rtos_close(int handle);

#endif /* _CLOSE_TRACE_H */
