/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_erff_fixed.c
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdbit.h>
#include <stdint.h>

#include "fixed.h"
#include "libm-fixed.h"

/* erf(i/8) and (2/sqrt(pi)) e^-(i/8)^2 in units of 2^-62, i = 0 .. 16 */
static const uint64_t below[17][2] = {
	{ 0x0000000000000000, 0x48375d410a6db447 }, /* 0 */
	{ 0x08faf0d1ae1dc4ba, 0x4718be87a1a6d8e7 }, /* 1 */
	{ 0x11af54e232d60890, 0x43d74580bdf90361 }, /* 2 */
	{ 0x19dd0d2b721f38ba, 0x3ebe19b5e2a62607 }, /* 3 */
	{ 0x214fdeb831dba458, 0x383df94934a022a6 }, /* 4 */
	{ 0x27e32db9a1e26983, 0x30dd2d2826973b62 }, /* 5 */
	{ 0x2d8392eb3a1cbed9, 0x2925c85af1a5893a }, /* 6 */
	{ 0x322e492a371f8b6c, 0x21956c3e10972642 }, /* 7 */
	{ 0x35eecf4e8211158e, 0x1a911f096fbc25be }, /* 8 */
	{ 0x38db5a5053d8c3cb, 0x145e99bcbb79155f }, /* 9 */
	{ 0x3b10cbb3157c0102, 0x0f23297d6e5b58fc }, /* 10 */
	{ 0x3caed1876947f831, 0x0ae72cae22ad850e }, /* 11 */
	{ 0x3dd4aaae26f5bf64, 0x079c8dc9ae092a37 }, /* 12 */
	{ 0x3e9ed276cf7aedb1, 0x0526751e2f29cd45 }, /* 13 */
	{ 0x3f25a0eff1aada63, 0x0360aa60644202ae }, /* 14 */
	{ 0x3f7cc3dde99ed48b, 0x02259d66fff3786d }, /* 15 */
	{ 0x3fb35c284f2bc666, 0x01529b9e8cf9a1e3 }, /* 16 */
};

/* R = erfc(x) e^(x^2) at x = 2 + i/8 in units of 2^-62, i = 0 .. 58 */
static const uint64_t scaled[59] = {
	0x1058671b52c775ec,
	0x0f87e947ee1055a9,
	0x0eca2236b13c98e5,
	0x0e1cc3e82685cbc2,
	0x0d7dd9f9db9a1d66,
	0x0cebb9c70fa6db8f,
	0x0c64f5b47f93eb49,
	0x0be852f4eeab8824,
	0x0b74c13e914e9667,
	0x0b0954092a28dead,
	0x0aa53d046a5da27d,
	0x0a47c78814cdb88b,
	0x09f054cd077489fa,
	0x099e58c8bcebfaff,
	0x0951578ce0a497c1,
	0x0908e312c92719c6,
	0x08c4995f8470aa21,
	0x088422f0ee58cd13,
	0x084731674644d644,
	0x080d7e611738dc08,
	0x07d6ca81273d3160,
	0x07a2dc97a43ee357,
	0x077180e8f9dfa6b1,
	0x0742888db524fa73,
	0x0715c8e7a50f4678,
	0x06eb1b290b86feca,
	0x06c25beb3851bf8d,
	0x069b6ad255d6db42,
	0x06762a3c8a13caea,
	0x06527efad8f5d84f,
	0x0630501273554778,
	0x060f86855164f567,
	0x05f00d212275c85d,
	0x05d1d053bff6b7f3,
	0x05b4be046fdce5d8,
	0x0598c5715c1bc2cf,
	0x057dd710ba5c5c26,
	0x0563e475315f326d,
	0x054ae03518f11be1,
	0x0532bdd43e8c8c3d,
	0x051b71afe403d9f3,
	0x0504f0ecb7350d35,
	0x04ef31668a0d630a,
	0x04da29a1993ffb73,
	0x04c5d0bd363534b0,
	0x04b21e67ae04ad1f,
	0x049f0ad34be261f3,
	0x048c8eac59627250,
	0x047aa310026d164f,
	0x0469418405c04066,
	0x045863ef1d7e2806,
	0x044804920d953caf,
	0x04381e0147d24741,
	0x0428ab1f1732a68c,
	0x0419a716459b05cd,
	0x040b0d553075dcd5,
	0x03fcd98941f2a70e,
	0x03ef079ac5b058ba,
	0x03e193a9108fc2ab
};

/* R'/R = 2x - (2/sqrt(pi))/R at the same points, in Q31 */
static const int32_t slope[59] = {
	-897993493,
	-858656932,
	-822301147,
	-788628443,
	-757376146,
	-728312076,
	-701230624,
	-675949356,
	-652306080,
	-630156304,
	-609371043,
	-589834919,
	-571444515,
	-554106948,
	-537738628,
	-522264181,
	-507615512,
	-493730980,
	-480554690,
	-468035857,
	-456128265,
	-444789778,
	-433981914,
	-423669477,
	-413820217,
	-404404541,
	-395395255,
	-386767327,
	-378497688,
	-370565043,
	-362949712,
	-355633482,
	-348599475,
	-341832032,
	-335316606,
	-329039668,
	-322988622,
	-317151726,
	-311518021,
	-306077273,
	-300819910,
	-295736971,
	-290820062,
	-286061310,
	-281453323,
	-276989157,
	-272662281,
	-268466548,
	-264396168,
	-260445682,
	-256609939,
	-252884076,
	-249263497,
	-245743854,
	-242321036,
	-238991147,
	-235750495,
	-232595581,
	-229523082
};

/* 1/k in units of 2^-32, k = 3 .. 13 */
static const int32_t inverse[11] = { 1431655765, 1073741824, 858993459, 715827883, 613566757, 536870912, 477218588, 429496730, 390451572, 357913941, 330382100 };

/* v 2^-shift rounded to nearest: a floor would leave the vanishing terms at -1 and bias the sum */
static int32_t rounded(int64_t v, int shift)
{
	return (int32_t)((v + (INT64_C(1) << (shift - 1))) >> shift);
}

/*
 * The sum over k >= 1 of the scaled Taylor terms q_k, (k + 1) q_{k+1} = a q_k +
 * b q_{k-1} with q_0 = 1 and q_1 = first, each divided by k + 1 when
 * integrate is set; q in Q(fraction), a and b in Q30 and b q_0 = b given
 * in Q(fraction) as base (it carries the first step), every product exact
 * and then rounded. 12 terms: past them the terms are below 2^-40 for
 * |a| <= 1.2 and |b| <= 2^-7.
 */
static int32_t series(int32_t first, int32_t a, int32_t b, int32_t base, bool integrate)
{
	int32_t previous = 0;
	int32_t current = first;
	int32_t sum = integrate ? rounded(first, 1) : first;

	for (int k = 1; k < 12; ++k) {
		int32_t bq = k == 1 ? base : rounded(__libc_fixed_wide_signed(b, previous), 30);
		int32_t next = rounded(__libc_fixed_wide_signed(a, current), 30) + bq;

		next = k == 1 ? rounded(next, 1) : rounded(__libc_fixed_wide_signed(next, inverse[k - 2]), 32);
		previous = current;
		current = next;
		sum += integrate ? rounded(__libc_fixed_wide_signed(current, inverse[k - 1]), 32) : current;
	}

	return sum;
}

/* A positive value v 2^(scale - 62) normalised to bit 63, the float exponent in *exponent */
static uint64_t normalise(uint64_t v, int scale, int *exponent)
{
	int shift = (int)stdc_leading_zeros(v);

	*exponent = scale + 1 - shift;
	return v << shift;
}

/*
 * erf(|x|) or erfc(x) in fixed point for 2^-19 <= |x| (PLAN D88). Below 2,
 * erf(x0 + h) = erf(x0) + g(x0) h (1 + S), x0 = i/8 the nearest node and
 * |h| <= 1/16, g = (2/sqrt(pi)) e^-x0^2 and S the integral series of
 * e^(-2 x0 t - t^2) (a = -2 x0 h, b = -2 h^2); at x0 = 0 erf is
 * (2/sqrt(pi)) x (1 + S) from x's own significand, which keeps x's relative
 * accuracy. From 2 to 9.25, erfc(x) = e^-x^2 R(x) with R = erfc e^(x^2):
 * R(x0 + h) = R(x0) (1 + T), T from (k + 1) r_{k+1} = 2 x0 r_k + 2 r_{k-1}
 * (from R' = 2xR - 2/sqrt(pi), with the tabulated R'/R as the first term),
 * and e^-x^2 from the exp2 core. erfc(-x) = 1 + erf(x) or 2 - erfc(x).
 * The result is v 2^(*exponent - 63) with bit 63 of v set; 0 where the
 * fixed-point path does not serve (|x| >= 9.25, or erfc below the normal
 * range). The error of the top 32 bits after the leading one, measured over
 * every float, is below __LIBC_FIXED_ERF_ERROR units.
 */
uint64_t __libc_erff_fixed(uint32_t bits, bool complement, int *exponent)
{
	int e = (int)((bits >> 23) & 0xff);
	uint32_t m = (bits & 0x007fffff) | 0x00800000;
	bool negative = (bits >> 31) != 0;

	if (e < 128) {
		/* |x| in units of 2^-34; the node i and h = |x| - i/8, |h| <= 2^30 */
		int64_t x34 = e >= 116 ? (int64_t)m << (e - 116) : (int64_t)(m >> (116 - e));
		int i = (int)((x34 + (INT64_C(1) << 30)) >> 31);
		int32_t h = (int32_t)(x34 - ((int64_t)i << 31));
		int32_t square = rounded(__libc_fixed_wide_signed(h, h), 32);
		int32_t s = series((int32_t)(-((int64_t)i * h) >> 4), (int32_t)(-((int64_t)i * h) >> 6), -(square >> 5), -rounded(square, 3), true);
		int64_t hs;
		uint64_t magnitude;
		uint64_t g;
		uint64_t product;
		uint64_t erf;

		if (i == 0 && !complement) {
			/* (2/sqrt(pi)) m (1 + S) 2^(e - 150): m 2^8 times 2/sqrt(pi) in Q31, S below 0 */
			uint64_t p = __libc_fixed_wide(m << 8, 0x906eba82);

			p += (uint64_t)(__libc_fixed_wide_signed((int32_t)(p >> 33), s) * 2);
			return normalise(p, e - 127, exponent);
		}

		/* h (1 + S) in units of 2^-62, below 2^59, times g in units of 2^-62 (four products of the 32-bit halves) */
		hs = ((int64_t)h << 28) + (__libc_fixed_wide_signed(h, s) >> 4);
		magnitude = hs < 0 ? -(uint64_t)hs : (uint64_t)hs;
		g = below[i][1];
		{
			uint32_t gh = (uint32_t)(g >> 32);
			uint32_t gl = (uint32_t)g;
			uint32_t mh = (uint32_t)(magnitude >> 32);
			uint32_t ml = (uint32_t)magnitude;

			product = (__libc_fixed_wide(gh, mh) << 2) + ((__libc_fixed_wide(gh, ml) + __libc_fixed_wide(gl, mh) + (__libc_fixed_wide(gl, ml) >> 32)) >> 30);
		}
		erf = hs < 0 ? below[i][0] - product : below[i][0] + product;

		if (!complement)
			return normalise(erf, 0, exponent);
		return normalise(negative ? (UINT64_C(1) << 62) + erf : (UINT64_C(1) << 62) - erf, 0, exponent);
	}

	if (e > 130)
		return 0;

	{
		/* x^2 in units of 2^-40 (to 4 bits below), then z = x^2 / ln 2 in units of 2^-37 */
		uint64_t x2 = __libc_fixed_wide(m, m) >> (260 - 2 * e);
		uint32_t xh = (uint32_t)(x2 >> 32);
		uint32_t xl = (uint32_t)x2;
		uint64_t z = __libc_fixed_wide(xh, 0x5c551d94) + ((__libc_fixed_wide(xh, 0xae0bf85e) + __libc_fixed_wide(xl, 0x5c551d94) + (__libc_fixed_wide(xl, 0xae0bf85e) >> 32)) >> 32);
		int64_t x34 = (int64_t)m << (e - 116);
		int i = (int)((x34 - (INT64_C(2) << 34) + (INT64_C(1) << 30)) >> 31);
		int32_t h = (int32_t)(x34 - (INT64_C(2) << 34) - ((int64_t)i << 31));
		int32_t square = rounded(__libc_fixed_wide_signed(h, h), 32);
		int32_t t;
		int n;
		uint32_t f;
		uint64_t r;

		if (i > 58)
			return 0;

		/* tau_1 = (R'/R) h in Q35 (every tau is below 0.03), a = 2 x0 h and b = 2 h^2 in Q30 with x0 = (16 + i)/8, and b in Q35 (h^2 in units of 2^-36, doubled) */
		t = series(rounded(__libc_fixed_wide_signed(slope[i], h), 30), (int32_t)(((int64_t)(16 + i) * h) >> 6), square >> 5, square, false);

		/* e^-x^2 = 2^n (1 + f 2^-32), and R(x0) (1 + T) (1 + f 2^-32) in units of 2^-62 */
		f = __libc_exp2f_fixed(-(int64_t)(z >> 1), &n);
		r = scaled[i];
		r += (uint64_t)(__libc_fixed_wide_signed((int32_t)(r >> 30), t) >> 5);
		r += __libc_fixed_wide((uint32_t)(r >> 32), f) + (__libc_fixed_wide((uint32_t)r, f) >> 32);

		if (complement && !negative) {
			uint64_t v = normalise(r, n, exponent);

			return *exponent < -126 ? 0 : v;
		}

		/* 1 - erfc(x) or 2 - erfc(x) in units of 2^-62; erfc(x) 2^-n is below 2^-7 */
		r = n > -62 ? r >> -n : 0;
		return normalise(complement ? (UINT64_C(1) << 63) - r : (UINT64_C(1) << 62) - r, 0, exponent);
	}
}
