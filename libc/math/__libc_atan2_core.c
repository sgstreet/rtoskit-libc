/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_atan2_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* atan(k/32), k = 0 ... 32, as double-doubles */
static const struct __libc_dd table[] = {
	{ .hi = 0x0.0p+0, .lo = 0x0.0p+0 },
	{ .hi = 0x1.ffd55bba97625p-6, .lo = -0x1.5ec431444912cp-60 },
	{ .hi = 0x1.ff55bb72cfdeap-5, .lo = -0x1.c934d86d23f1dp-60 },
	{ .hi = 0x1.7ee182602f10fp-4, .lo = -0x1.cfb654c0c3d98p-58 },
	{ .hi = 0x1.fd5ba9aac2f6ep-4, .lo = -0x1.cd37686760c17p-59 },
	{ .hi = 0x1.3d6eee8c6626cp-3, .lo = 0x1.61a3b0ce9281bp-57 },
	{ .hi = 0x1.7b97b4bce5b02p-3, .lo = 0x1.347b0b4f881cap-58 },
	{ .hi = 0x1.b90d7529260a2p-3, .lo = 0x1.17b10d2e0e5abp-61 },
	{ .hi = 0x1.f5b75f92c80ddp-3, .lo = 0x1.8ab6e3cf7afbdp-57 },
	{ .hi = 0x1.18bf5a30bf178p-2, .lo = 0x1.30ca4748b1bf9p-57 },
	{ .hi = 0x1.362773707ebccp-2, .lo = -0x1.963a544b672d8p-57 },
	{ .hi = 0x1.530ad9951cd4ap-2, .lo = -0x1.2566480884082p-57 },
	{ .hi = 0x1.6f61941e4def1p-2, .lo = -0x1.c63aae6f6e918p-56 },
	{ .hi = 0x1.8b24d394a1b25p-2, .lo = 0x1.b6d0ba3748fa8p-56 },
	{ .hi = 0x1.a64eec3cc23fdp-2, .lo = -0x1.24dec1b50b7ffp-56 },
	{ .hi = 0x1.c0db4c94ec9f0p-2, .lo = -0x1.cc1ce70934c34p-56 },
	{ .hi = 0x1.dac670561bb4fp-2, .lo = 0x1.a2b7f222f65e2p-56 },
	{ .hi = 0x1.f40dd0b541418p-2, .lo = -0x1.a3992dc382a23p-57 },
	{ .hi = 0x1.0657e94db30d0p-1, .lo = -0x1.d5b495f6349e6p-56 },
	{ .hi = 0x1.1255d9bfbd2a9p-1, .lo = -0x1.2bdaee1c0ee35p-58 },
	{ .hi = 0x1.1e00babdefeb4p-1, .lo = -0x1.928df287a668fp-58 },
	{ .hi = 0x1.2958e59308e31p-1, .lo = -0x1.09e73b0c6c087p-56 },
	{ .hi = 0x1.345f01cce37bbp-1, .lo = 0x1.1021137c71102p-55 },
	{ .hi = 0x1.3f13fb89e96f4p-1, .lo = 0x1.ecf8b492644f0p-56 },
	{ .hi = 0x1.4978fa3269ee1p-1, .lo = 0x1.2419a87f2a458p-56 },
	{ .hi = 0x1.538f57b89061fp-1, .lo = -0x1.1bb74abda520cp-55 },
	{ .hi = 0x1.5d58987169b18p-1, .lo = 0x1.0028e4bc5e7cap-57 },
	{ .hi = 0x1.66d663923e087p-1, .lo = -0x1.6ea6febe8bbbap-56 },
	{ .hi = 0x1.700a7c5784634p-1, .lo = -0x1.8c34d25aadef6p-56 },
	{ .hi = 0x1.78f6bbd5d315ep-1, .lo = 0x1.406a089803740p-55 },
	{ .hi = 0x1.819d0b7158a4dp-1, .lo = -0x1.bf76229d3b917p-56 },
	{ .hi = 0x1.89ff5ff57f1f8p-1, .lo = -0x1.55b9a5e177a1bp-55 },
	{ .hi = 0x1.921fb54442d18p-1, .lo = 0x1.1a62633145c07p-55 },
};

/*
 * atan(t) for t in [0, 1] (Tang's table method): c = k/32 nearest to t,
 * atan(t) = atan(c) + atan(u) with u = (t - c)/(1 + tc), |u| <= 1/64 (t - c is
 * exact by Sterbenz), and atan(u) the series to u^11 (remainder below 2^-72
 * relative). Below 2^-30, atan(t) is t to 2^-60 relative (and the cube would
 * underflow).
 */
static struct __libc_dd atan_unit(struct __libc_dd t)
{
	int k = (int)(t.hi * 32.0 + 0.5);
	double c = k / 32.0;
	struct __libc_dd numerator = __libc_dd_fast_sum(t.hi - c, t.lo);
	struct __libc_dd product = __libc_dd_product(t.hi, c);
	struct __libc_dd denominator = __libc_dd_fast_sum(1.0, product.hi);
	struct __libc_dd u;
	double v;
	double tail;
	struct __libc_dd sum;

	if (t.hi < 0x1p-30)
		return t;

	denominator = __libc_dd_fast_sum(denominator.hi, denominator.lo + product.lo + t.lo * c);
	u = __libc_dd_divide(numerator, denominator);
	v = u.hi * u.hi;
	tail = u.hi * v * (-0x1.5555555555555p-2 + v * (0x1.999999999999ap-3 + v * (-0x1.2492492492492p-3 + v * (0x1.c71c71c71c71cp-4 + v * -0x1.745d1745d1746p-4))));
	sum = __libc_dd_sum(table[k].hi, u.hi);

	return __libc_dd_fast_sum(sum.hi, sum.lo + table[k].lo + u.lo + tail);
}

/*
 * The octant reduction: the smaller coordinate over the larger, then
 * pi/2 - a when they were swapped, pi - a for a negative x, and the sign of
 * y. Rounding to nearest is expected (D47).
 */
struct __libc_dd __libc_atan2_core(struct __libc_dd y, struct __libc_dd x)
{
	struct __libc_dd ay = signbit(y.hi) ? __libc_dd_negate(y) : y;
	struct __libc_dd ax = signbit(x.hi) ? __libc_dd_negate(x) : x;
	bool swap = ay.hi > ax.hi;
	struct __libc_dd angle = swap ? atan_unit(__libc_dd_divide(ax, ay)) : atan_unit(__libc_dd_divide(ay, ax));

	if (swap)
		angle = __libc_dd_add(__LIBC_PI_2, __libc_dd_negate(angle));
	if (signbit(x.hi))
		angle = __libc_dd_add(__LIBC_PI, __libc_dd_negate(angle));

	return signbit(y.hi) ? __libc_dd_negate(angle) : angle;
}
