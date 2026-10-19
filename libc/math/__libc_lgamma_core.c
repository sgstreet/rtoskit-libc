/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_lgamma_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* zeta(k) - 1 for k = 2 ... 40 */
static const double zeta_minus_one[] = { 0x1.4a34cc4a60fa6p-1, 0x1.9dd002780310ap-3, 0x1.51322ac7d8483p-4, 0x1.2e831d94f99b7p-5, 0x1.1c26130249124p-6, 0x1.1196d0a679c47p-7, 0x1.0b36af86396e9p-8, 0x1.073e7b02d6ae0p-9, 0x1.04b8ce96ee5f8p-10, 0x1.0318df2459954p-11, 0x1.020a5b2cd3042p-12, 0x1.01593a1177bd6p-13, 0x1.00e4af2b4e156p-14, 0x1.0097bcbf11bedp-15, 0x1.0064cdeb22f0fp-16, 0x1.0043073686681p-17, 0x1.002c9953744ccp-18, 0x1.001db08f9ba4ap-19, 0x1.0013c594466eap-20, 0x1.000d2bab28121p-21, 0x1.0008c66cec77dp-22, 0x1.0005d8f13858cp-23, 0x1.0003e59ffde12p-24, 0x1.000298ea55633p-25, 0x1.0001bb316ccdap-26, 0x1.0001276b90845p-27, 0x1.0000c4ed05ae3p-28, 0x1.0000834601a87p-29, 0x1.00005782aaebep-30, 0x1.00003a56719b9p-31, 0x1.000026e3f644fp-32, 0x1.000019ed24201p-33, 0x1.00001148ad653p-34, 0x1.00000b85be414p-35, 0x1.000007ae797ffp-36, 0x1.0000051ef8ffap-37, 0x1.00000369f9ff9p-38, 0x1.00000246a5ffbp-39, 0x1.000001846e551p-40 };

static struct __libc_dd log_dd(struct __libc_dd a)
{
	struct __libc_dd log = __libc_log_core(a.hi);

	return __libc_dd_fast_sum(log.hi, log.lo + a.lo / a.hi);
}

/*
 * Stirling's series for y >= 12 (a double-double): (y - 1/2) log y - y +
 * log(2 pi)/2 + sum B_2k / (2k (2k - 1) y^(2k - 1)), ten terms (the next is
 * below 2^-66 of the result at 12).
 */
static struct __libc_dd stirling(struct __libc_dd y)
{
	const struct __libc_dd half_log_2pi = { .hi = 0x1.d67f1c864beb5p-1, .lo = -0x1.65b5a1b7ff5dfp-55 };
	double z = 1.0 / y.hi;
	double z2 = z * z;
	double series = z * (0x1.5555555555555p-4 + z2 * (-0x1.6c16c16c16c17p-9 + z2 * (0x1.a01a01a01a01ap-11 + z2 * (-0x1.3813813813814p-11 + z2 * (0x1.b951e2b18ff23p-11 + z2 * (-0x1.f6ab0d9993c7dp-10 + z2 * (0x1.a41a41a41a41ap-8 + z2 * (-0x1.e4286cb0f5398p-6 + z2 * (0x1.6fe96381e0680p-3 + z2 * (-0x1.6476701181f3ap+0))))))))));
	struct __libc_dd value = __libc_dd_multiply(__libc_dd_add(y, (struct __libc_dd){ .hi = -0.5, .lo = 0.0 }), log_dd(y));

	value = __libc_dd_add(value, __libc_dd_negate(y));
	value = __libc_dd_add(value, half_log_2pi);

	return __libc_dd_fast_sum(value.hi, value.lo + series);
}

/*
 * lgamma(1 + z) = -gamma z + sum_(k >= 2) (-1)^k zeta(k) z^k / k and
 * lgamma(2 + z) = (1 - gamma) z + sum_(k >= 2) (-1)^k (zeta(k) - 1) z^k / k for
 * |z| <= 1/4, keeping the relative accuracy at the zeros 1 and 2; the first
 * two terms as double-doubles, the rest to k = 40 (zeta(k) - 1 is below
 * 2^-(k - 1)).
 */
static struct __libc_dd near_zero(struct __libc_dd z, bool two)
{
	const struct __libc_dd euler = { .hi = 0x1.2788cfc6fb619p-1, .lo = -0x1.6cb90701fbfabp-58 };
	const struct __libc_dd one_minus_euler = { .hi = 0x1.b0ee6072093cep-2, .lo = 0x1.6cb90701fbfabp-58 };
	const struct __libc_dd zeta2_half = { .hi = 0x1.a51a6625307d3p-1, .lo = 0x1.1873d8912200cp-56 };
	const struct __libc_dd zeta2m1_half = { .hi = 0x1.4a34cc4a60fa6p-2, .lo = 0x1.1873d8912200cp-56 };
	double tail = 0.0;
	struct __libc_dd first;
	struct __libc_dd second;

	for (int k = 40; k >= 3; --k) {
		double coefficient = (two ? zeta_minus_one[k - 2] : 1.0 + zeta_minus_one[k - 2]) / k;

		tail = tail * z.hi + ((k & 1) != 0 ? -coefficient : coefficient);
	}
	first = __libc_dd_multiply(two ? one_minus_euler : __libc_dd_negate(euler), z);
	second = __libc_dd_multiply(two ? zeta2m1_half : zeta2_half, __libc_dd_multiply(z, z));
	first = __libc_dd_add(first, second);

	return __libc_dd_fast_sum(first.hi, first.lo + z.hi * z.hi * z.hi * tail);
}

/* log gamma(x) for a positive double-double x below 2^60 */
static struct __libc_dd lgamma_positive(struct __libc_dd x)
{
	struct __libc_dd product;
	struct __libc_dd shifted;
	int n;

	if (x.hi >= 12.0)
		return stirling(x);
	if (fabs(x.hi - 1.0) <= 0.25)
		return near_zero(__libc_dd_add(x, (struct __libc_dd){ .hi = -1.0, .lo = 0.0 }), false);
	if (fabs(x.hi - 2.0) <= 0.25)
		return near_zero(__libc_dd_add(x, (struct __libc_dd){ .hi = -2.0, .lo = 0.0 }), true);

	/* gamma(x) = gamma(x + n) / (x (x + 1) ... (x + n - 1)) with x + n >= 12 */
	n = (int)ceil(12.0 - x.hi);
	product = x;
	for (int k = 1; k < n; ++k)
		product = __libc_dd_multiply(product, __libc_dd_add(x, (struct __libc_dd){ .hi = k, .lo = 0.0 }));
	shifted = __libc_dd_add(x, (struct __libc_dd){ .hi = n, .lo = 0.0 });

	return __libc_dd_add(stirling(shifted), __libc_dd_negate(log_dd(product)));
}

/*
 * Positive x directly; negative x by the reflection
 * gamma(x) gamma(1 - x) = pi / sin(pi x): log pi - log|sin(pi x)| -
 * lgamma(1 - x), with 1 - x exact as a double-double and sin(pi x) from the
 * exact reduction x = n + f, |f| <= 1/2.
 */
struct __libc_dd __libc_lgamma_core(double x, int *sign)
{
	const struct __libc_dd log_pi = { .hi = 0x1.250d048e7a1bdp+0, .lo = 0x1.7abf2ad8d5088p-57 };
	double n;
	double f;
	double a;
	struct __libc_dd angle;
	struct __libc_dd sine;
	struct __libc_dd value;

	*sign = 1;
	if (x > 0)
		return lgamma_positive((struct __libc_dd){ .hi = x, .lo = 0.0 });

	n = roundeven(x);
	f = x - n;
	a = fabs(f);
	*sign = (f < 0) != (fmod(n, 2.0) != 0) ? -1 : 1;
	if (a <= 0.25) {
		angle = __libc_dd_product(a, __LIBC_PI.hi);
		angle = __libc_dd_fast_sum(angle.hi, angle.lo + a * __LIBC_PI.lo);
		sine = __libc_sin_core(angle);
	} else {
		angle = __libc_dd_product(0.5 - a, __LIBC_PI.hi);
		angle = __libc_dd_fast_sum(angle.hi, angle.lo + (0.5 - a) * __LIBC_PI.lo);
		sine = __libc_cos_core(angle);
	}
	value = __libc_dd_add(log_pi, __libc_dd_negate(log_dd(sine)));

	return __libc_dd_add(value, __libc_dd_negate(lgamma_positive(__libc_dd_sum(1.0, -x))));
}
