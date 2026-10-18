/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * strerror.c
 *
 * Created on: Oct 18, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <string.h>

/*
 * C23 7.26.6.3: a switch keeps the messages for the sparse Linux numbering
 * (D20) out of a mostly empty pointer table. The table grows with <errno.h>.
 */
char *strerror(int errnum)
{
	switch (errnum) {
		case 0:
			return "No error";
		case EBADF:
			return "Bad file descriptor";
		case EINVAL:
			return "Invalid argument";
		case EDOM:
			return "Numerical argument out of domain";
		case ERANGE:
			return "Numerical result out of range";
		case EOVERFLOW:
			return "Value too large for defined data type";
		case EILSEQ:
			return "Invalid or incomplete multibyte or wide character";
		default:
			return "Unknown error";
	}
}
