/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * localeconv.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <limits.h>
#include <locale.h>

/*
 * C23 7.11.2.1p5: the "C" locale's values. The program must not modify the
 * structure, and setlocale never changes it, so one shared constant object
 * serves every thread.
 */
static const struct lconv c_locale = {
	.decimal_point = (char *)".",
	.thousands_sep = (char *)"",
	.grouping = (char *)"",
	.mon_decimal_point = (char *)"",
	.mon_thousands_sep = (char *)"",
	.mon_grouping = (char *)"",
	.positive_sign = (char *)"",
	.negative_sign = (char *)"",
	.currency_symbol = (char *)"",
	.frac_digits = CHAR_MAX,
	.p_cs_precedes = CHAR_MAX,
	.n_cs_precedes = CHAR_MAX,
	.p_sep_by_space = CHAR_MAX,
	.n_sep_by_space = CHAR_MAX,
	.p_sign_posn = CHAR_MAX,
	.n_sign_posn = CHAR_MAX,
	.int_curr_symbol = (char *)"",
	.int_frac_digits = CHAR_MAX,
	.int_p_cs_precedes = CHAR_MAX,
	.int_n_cs_precedes = CHAR_MAX,
	.int_p_sep_by_space = CHAR_MAX,
	.int_n_sep_by_space = CHAR_MAX,
	.int_p_sign_posn = CHAR_MAX,
	.int_n_sign_posn = CHAR_MAX,
};

struct lconv *localeconv(void)
{
	return (struct lconv *)&c_locale;
}
