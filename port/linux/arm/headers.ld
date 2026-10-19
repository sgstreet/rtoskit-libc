/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * headers.ld
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * Added to the default arm-none-eabi linker script (-T with INSERT) by the
 * linux-<arch> boards. The default script starts the code at the text
 * segment base, so the ELF and program headers are not loaded. Leaving room
 * for them puts them in the first segment, which defines __ehdr_start for
 * the port's search of the program headers for PT_TLS.
 */
SECTIONS
{
	. = SEGMENT_START("text-segment", 0x8000) + SIZEOF_HEADERS;
}
INSERT BEFORE .init;
