/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_lgammaf_core.c
 *
 * Created on: Oct 20, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* lgamma(1 + z) = sum c_k z^k: c_1 = -Euler's gamma, c_k = (-1)^k zeta(k)/k; c_1 .. c_17 as float-float */
static const float pairs[17][2] = {
	{ -0x1.2788dp-1f, 0x1.c824f4p-28f }, /* 1 */
	{ 0x1.a51a66p-1f, 0x1.2983eap-28f }, /* 2 */
	{ -0x1.9a4d56p-2f, 0x1.05534ap-28f }, /* 3 */
	{ 0x1.151322p-2f, 0x1.58fb0ap-27f }, /* 4 */
	{ -0x1.a8b9c2p-3f, 0x1.0ab3d6p-28f }, /* 5 */
	{ 0x1.5b40ccp-3f, -0x1.dfe7ap-28f }, /* 6 */
	{ -0x1.2703a2p-3f, 0x1.18ae28p-30f }, /* 7 */
	{ 0x1.010b36p-3f, 0x1.5f0c72p-28f }, /* 8 */
	{ -0x1.c8067p-4f, -0x1.b55f6ep-30f }, /* 9 */
	{ 0x1.9a01e4p-4f, -0x1.e8a81cp-30f }, /* 10 */
	{ -0x1.748c34p-4f, 0x1.dd6726p-29f }, /* 11 */
	{ 0x1.556ad6p-4f, 0x1.921de2p-31f }, /* 12 */
	{ -0x1.3b1d98p-4f, 0x1.c074dp-29f }, /* 13 */
	{ 0x1.2496ep-4f, -0x1.f37ce8p-30f }, /* 14 */
	{ -0x1.111334p-4f, -0x1.db9ff8p-30f }, /* 15 */
	{ 0x1.0001p-4f, 0x1.9337acp-30f }, /* 16 */
	{ -0x1.e1e2d4p-5f, 0x1.dc2ea8p-30f }, /* 17 */
};

/* c_18 .. c_44 */
static const float tail[27] = { 0x1.c71ce4p-5f, -0x1.af28a2p-5f, 0x1.9999b4p-5f, -0x1.86186ep-5f, 0x1.745d1ep-5f, -0x1.642c88p-5f, 0x1.555556p-5f, -0x1.47ae16p-5f, 0x1.3b13b2p-5f, -0x1.2f684cp-5f, 0x1.24924ap-5f, -0x1.1a7b96p-5f, 0x1.111112p-5f, -0x1.08421p-5f, 0x1p-5f, -0x1.f07c2p-6f, 0x1.e1e1e2p-6f, -0x1.d41d42p-6f, 0x1.c71c72p-6f, -0x1.bacf92p-6f, 0x1.af286cp-6f, -0x1.a41a42p-6f, 0x1.99999ap-6f, -0x1.8f9c18p-6f, 0x1.861862p-6f, -0x1.7d05f4p-6f, 0x1.745d18p-6f };

/*
 * lgamma(1 + z) for |z| <= 1/2 (z exact): the series to z^44 (its remainder
 * is below 2^-46 of the result at |z| = 1/2), Horner in float from z^44 down
 * to z^18 (their rounding stays below 2^-44 of the result) and in
 * float-float below.
 */
static struct __libc_ff lgamma1p(float z)
{
	float q = 0.0f;
	struct __libc_ff sum;

	for (int k = 26; k >= 0; --k)
		q = q * z + tail[k];
	sum = (struct __libc_ff){ .hi = q, .lo = 0.0f };
	for (int k = 16; k >= 0; --k)
		sum = __libc_ff_add((struct __libc_ff){ .hi = pairs[k][0], .lo = pairs[k][1] }, __libc_ff_multiply(sum, (struct __libc_ff){ .hi = z, .lo = 0.0f }));

	return __libc_ff_multiply(sum, (struct __libc_ff){ .hi = z, .lo = 0.0f });
}

/*
 * The zeros of lgamma in (-9, -2), two in each interval between poles, with
 * the Taylor coefficients c_k = psi^(k-1)(z)/k! of lgamma there to c_6 (PLAN D68),
 * c_1 and c_2 as float-float. The zero is three floats, so x - z is exact to 2^-72.
 * Within radius of a zero, where |lgamma| < 2^-8 and the reflection would
 * cancel, lgamma is the series in x - z (its truncation stays below 2^-43).
 */
static const struct {
	float zero[3];
	float radius;
	float c1[2];
	float c2[2];
	float c[4];
} zeros[14] = {
	{ { -0x1.5fb41p1f, -0x1.437b2p-24f, -0x1.97995ap-52f }, 0x1.0b7424p-9f, { -0x1.ea12dap0f, -0x1.209632p-25f }, { 0x1.3267f4p3f, -0x1.ecd2d8p-24f }, { -0x1.4185acp4f, 0x1.f504acp5f, -0x1.858846p7f, 0x1.4373f8p9f } }, /* -2.74768264673 */
	{ { -0x1.3a7fcap1f, 0x1.3fe0f2p-24f, 0x1.f5504ep-50f }, 0x1.51d1bap-9f, { 0x1.83fe96p0f, 0x1.abd4d8p-26f }, { 0x1.36eebcp2f, -0x1.ffa154p-23f }, { 0x1.694a6p0f, 0x1.1718d8p3f, 0x1.7339fep2f, 0x1.8d32f6p4f } }, /* -2.45702473822 */
	{ { -0x1.fa4716p1f, 0x1.707a04p-24f, -0x1.42e1a8p-49f }, 0x1.8b453p-13f, { -0x1.4b99dap4f, 0x1.327538p-21f }, { 0x1.f76deap7f, 0x1.c086d8p-18f }, { -0x1.d2535ap11f, 0x1.e8f82ap15f, -0x1.116f78p20f, 0x1.3e8f3ap24f } }, /* -3.95529428486 */
	{ { -0x1.9260dcp1f, 0x1.b0d328p-26f, 0x1.04741ap-52f }, 0x1.072ce2p-11f, { 0x1.f20a66p2f, -0x1.a0a756p-27f }, { 0x1.9d4d2ap4f, -0x1.11d5e2p-21f }, { 0x1.c11372p6f, 0x1.267204p9f, 0x1.99a634p11f, 0x1.293c4p14f } }, /* -3.14358088835 */
	{ { -0x1.3f7578p2f, 0x1.64454p-24f, 0x1.95de5ep-49f }, 0x1.192f1cp-15f, { -0x1.d224a4p6f, 0x1.061bep-22f }, { 0x1.b533c6p12f, 0x1.e28e56p-14f }, { -0x1.0d3f8p19f, 0x1.752a7p25f, -0x1.13d5d2p32f, 0x1.a8c5c6p38f } }, /* -4.99154464056 */
	{ { -0x1.0284e8p2f, 0x1.e99aap-24f, -0x1.861cf8p-51f }, 0x1.31c7bep-13f, { 0x1.aca5dp4f, -0x1.6dbd38p-21f }, { 0x1.44415cp8f, 0x1.b027f2p-17f }, { 0x1.559b12p12f, 0x1.96d18ep16f, 0x1.0261ecp21f, 0x1.55e3dcp25f } }, /* -4.03936183974 */
	{ { -0x1.7fe93p2f, 0x1.4dc17ep-23f, 0x1.504562p-49f }, 0x1.6dff58p-18f, { -0x1.661f6ap9f, -0x1.0e9784p-17f }, { 0x1.f79dccp17f, -0x1.0d61b2p-8f }, { -0x1.d6e808p26f, 0x1.ef5d3p35f, -0x1.15ea6cp45f, 0x1.44d54ep54f } }, /* -5.99860748008 */
	{ { -0x1.4086a6p2f, 0x1.01e924p-23f, 0x1.bcd5b4p-48f }, 0x1.099fd6p-15f, { 0x1.ed72ep6f, 0x1.0535cp-19f }, { 0x1.cecc32p12f, 0x1.d845f4p-13f }, { 0x1.253d86p19f, 0x1.a225ep25f, 0x1.3e0178p32f, 0x1.f7d8d6p38f } }, /* -5.00821816832 */
	{ { -0x1.bffccp2f, 0x1.128f22p-23f, 0x1.853b2ap-57f }, 0x1.a06f4ep-21f, { -0x1.3abf7ap12f, -0x1.73aa46p-14f }, { 0x1.8349a2p23f, 0x1.54108cp-3f }, { -0x1.3d91dap35f, 0x1.24f3d6p47f, -0x1.20427ep59f, 0x1.2775e8p71f } }, /* -6.99980150789 */
	{ { -0x1.8016b2p2f, -0x1.625f24p-24f, 0x1.893f08p-49f }, 0x1.6a3588p-18f, { 0x1.69de4ap9f, -0x1.c50d56p-19f }, { 0x1.fce234p17f, 0x1.099fa2p-8f }, { 0x1.de503ap26f, 0x1.f9c7b6p35f, 0x1.1d3d5p45f, 0x1.4f21e2p54f } }, /* -6.00138529445 */
	{ { -0x1.ffff98p2f, 0x1.fa98c4p-28f, -0x1.e54f42p-55f }, 0x1.a02552p-24f, { -0x1.3af77p15f, 0x1.b3d054p-13f }, { 0x1.838e76p29f, 0x1.955e24p4f }, { -0x1.3de68cp44f, 0x1.255c06p59f, -0x1.20c2a8p74f, 0x1.281394p89f } }, /* -7.9999751971 */
	{ { -0x1.c0034p2f, 0x1.0900fp-25f, 0x1.482eep-50f }, 0x1.9fc4e6p-21f, { 0x1.3b407ap12f, 0x1.470f7ap-13f }, { 0x1.83e85ep23f, -0x1.54114ap-3f }, { 0x1.3e552cp35f, 0x1.25e42ap47f, 0x1.216a36p59f, 0x1.28e1c8p71f } }, /* -7.00019833341 */
	{ { -0x1.1ffffap3f, -0x1.c425e8p-24f, -0x1.ff90cap-53f }, 0x1.71df68p-27f, { -0x1.625eep18f, 0x1.ce1268p-9f }, { 0x1.ea8c16p35f, -0x1.f6febp10f }, { -0x1.c4b30ep53f, 0x1.d5fe46p71f, -0x1.043d22p90f, 0x1.2c334ap108f } }, /* -8.99999724425 */
	{ { -0x1.000034p3f, -0x1.459fcap-28f, 0x1.3e6986p-57f }, 0x1.a00eb2p-24f, { 0x1.3b089p15f, -0x1.2988e8p-13f }, { 0x1.83a38ap29f, -0x1.955e24p4f }, { 0x1.3e0078p44f, 0x1.257becp59f, 0x1.20e9eap74f, 0x1.2843e2p89f } }, /* -8.00002480027 */
};

/* ln of a positive float-float */
static struct __libc_ff ln(struct __libc_ff a)
{
	return __libc_ff_multiply(__libc_log2f_core(a.hi, a.lo), __LIBC_LN2F);
}

/*
 * Stirling's series for y >= 8: (y - 1/2) ln y - y + ln(2 pi)/2 + 1/(12y) -
 * 1/(360y^3) + 1/(1260y^5) - 1/(1680y^7) + 1/(1188y^9) (the next term is
 * below 2^-45 of the result at 8), 1/(12y) as float-float; far up, y (ln y -
 * 1) scaled by 2^-64 so that the exact products cannot overflow, and the
 * rest is below 2^-50 of it.
 */
static struct __libc_ff stirling(float y)
{
	struct __libc_ff log = ln((struct __libc_ff){ .hi = y, .lo = 0.0f });
	struct __libc_ff value;
	float r;
	float r2;

	if (y > 0x1p60f) {
		value = __libc_ff_multiply((struct __libc_ff){ .hi = scalbnf(y, -64), .lo = 0.0f }, __libc_ff_add(log, (struct __libc_ff){ .hi = -1.0f, .lo = 0.0f }));
		return (struct __libc_ff){ .hi = scalbnf(value.hi, 64), .lo = scalbnf(value.lo, 64) };
	}

	r = 1.0f / y;
	r2 = r * r;
	value = __libc_ff_multiply(__libc_ff_sum(y, -0.5f), log);
	value = __libc_ff_add(value, (struct __libc_ff){ .hi = -y, .lo = 0.0f });
	value = __libc_ff_add(value, __LIBC_HALF_LN_2PIF);
	value = __libc_ff_add(value, __libc_ff_divide(__LIBC_ONE_TWELFTHF, (struct __libc_ff){ .hi = y, .lo = 0.0f }));

	return __libc_ff_add(value, (struct __libc_ff){ .hi = r * r2 * (-0x1.6c16c2p-9f + r2 * (0x1.a01a02p-11f + r2 * (-0x1.381382p-11f + r2 * 0x1.b951e2p-11f))), .lo = 0.0f });
}

/* lgamma(x) for x within the radius of a zero in (-9, -2), else false */
static bool near_zero(float x, struct __libc_ff *value)
{
	int n = (int)-x;
	int index = 2 * (n - 2) + (x > -(float)n - 0.5f ? 1 : 0);
	float offset = x - zeros[index].zero[0];
	struct __libc_ff d;
	struct __libc_ff sum;
	float q;

	if (!(fabsf(offset) < zeros[index].radius))
		return false;

	/* x - z.hi is exact (Sterbenz); the other two parts of z follow as float-float */
	d = __libc_ff_add(__libc_ff_sum(offset, -zeros[index].zero[1]), (struct __libc_ff){ .hi = -zeros[index].zero[2], .lo = 0.0f });
	q = zeros[index].c[3];
	for (int k = 2; k >= 0; --k)
		q = q * d.hi + zeros[index].c[k];
	sum = __libc_ff_add((struct __libc_ff){ .hi = zeros[index].c2[0], .lo = zeros[index].c2[1] }, __libc_ff_multiply((struct __libc_ff){ .hi = q, .lo = 0.0f }, d));
	sum = __libc_ff_add((struct __libc_ff){ .hi = zeros[index].c1[0], .lo = zeros[index].c1[1] }, __libc_ff_multiply(sum, d));
	*value = __libc_ff_multiply(sum, d);

	return true;
}

/*
 * log|gamma(x)| for finite x that is not a pole, with *sign the sign of
 * gamma(x) (PLAN D64):
 * - from 8 up, Stirling;
 * - [2.5, 8): down to [1.5, 2.5) by gamma(x) = (x - 1) gamma(x - 1), every
 *   x - k exact, so the logarithm of a product of positive exact factors is
 *   added (no cancellation);
 * - [0.5, 2.5): the zeta series at 1 + z, with log1p(z) added near 2;
 * - (-1/2, 1/2): lgamma(1 + x) - log|x|, x exact;
 * - within 2^-8/|psi| of a zero in (-9, -2), its Taylor series;
 * - below -1/2: the reflection gamma(x) gamma(-x) = -pi/(x sin(pi x)) with
 *   -x exact and sin(pi x) from the sin core after an exact reduction.
 */
struct __libc_ff __libc_lgammaf_core(float x, int *sign)
{
	struct __libc_ff value;
	struct __libc_ff product;
	struct __libc_ff sine;
	float y;
	float n;
	int k;

	*sign = 1;
	if (x >= 8.0f)
		return stirling(x);
	if (x >= 2.5f) {
		k = (int)(x - 1.5f);
		y = x - (float)k;
		product = (struct __libc_ff){ .hi = x - 1.0f, .lo = 0.0f };
		for (int i = 2; i <= k; ++i)
			product = __libc_ff_multiply(product, (struct __libc_ff){ .hi = x - (float)i, .lo = 0.0f });
		return __libc_ff_add(__libc_lgammaf_core(y, sign), ln(product));
	}
	if (x >= 1.5f)
		return __libc_ff_add(lgamma1p(x - 2.0f), __libc_ff_multiply(__libc_log2p1f_core(x - 2.0f), __LIBC_LN2F));
	if (x >= 0.5f)
		return lgamma1p(x - 1.0f);
	if (x > -0.5f) {
		*sign = x < 0 ? -1 : 1;
		return __libc_ff_add(lgamma1p(x), (struct __libc_ff){ .hi = -ln((struct __libc_ff){ .hi = fabsf(x), .lo = 0.0f }).hi, .lo = -ln((struct __libc_ff){ .hi = fabsf(x), .lo = 0.0f }).lo });
	}

	/* gamma is negative on (-n - 1, -n) for even n */
	if (x > -9.0f && x < -2.0f && near_zero(x, &value)) {
		*sign = ((int)-x & 1) != 0 ? 1 : -1;
		return value;
	}

	/* 32 x = K + d exactly (|x| below 2^23): sin(pi x) = sin(K pi/32 + d pi/32) */
	n = roundf(32.0f * x);
	sine = __libc_sinf_core((int)n, __libc_ff_scale(32.0f * x - n, __LIBC_PI_32F));
	*sign = sine.hi < 0 ? -1 : 1;
	if (sine.hi < 0)
		sine = (struct __libc_ff){ .hi = -sine.hi, .lo = -sine.lo };
	value = __libc_lgammaf_core(-x, &k);
	value = __libc_ff_add(value, ln(__libc_ff_multiply(sine, (struct __libc_ff){ .hi = -x, .lo = 0.0f })));

	return __libc_ff_add(__LIBC_LN_PIF, (struct __libc_ff){ .hi = -value.hi, .lo = -value.lo });
}
