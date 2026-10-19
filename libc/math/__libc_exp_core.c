/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_exp_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* 2^(j/64), j = 0..63, as double-double (computed to 70 digits) */
static const struct __libc_dd table[64] = {
	{ 0x1.0000000000000p+0, 0x0.0p+0 },
	{ 0x1.02c9a3e778061p+0, -0x1.19083535b085dp-56 },
	{ 0x1.059b0d3158574p+0, 0x1.d73e2a475b465p-55 },
	{ 0x1.0874518759bc8p+0, 0x1.186be4bb284ffp-57 },
	{ 0x1.0b5586cf9890fp+0, 0x1.8a62e4adc610bp-54 },
	{ 0x1.0e3ec32d3d1a2p+0, 0x1.03a1727c57b53p-59 },
	{ 0x1.11301d0125b51p+0, -0x1.6c51039449b3ap-54 },
	{ 0x1.1429aaea92de0p+0, -0x1.32fbf9af1369ep-54 },
	{ 0x1.172b83c7d517bp+0, -0x1.19041b9d78a76p-55 },
	{ 0x1.1a35beb6fcb75p+0, 0x1.e5b4c7b4968e4p-55 },
	{ 0x1.1d4873168b9aap+0, 0x1.e016e00a2643cp-54 },
	{ 0x1.2063b88628cd6p+0, 0x1.dc775814a8495p-55 },
	{ 0x1.2387a6e756238p+0, 0x1.9b07eb6c70573p-54 },
	{ 0x1.26b4565e27cddp+0, 0x1.2bd339940e9d9p-55 },
	{ 0x1.29e9df51fdee1p+0, 0x1.612e8afad1255p-55 },
	{ 0x1.2d285a6e4030bp+0, 0x1.0024754db41d5p-54 },
	{ 0x1.306fe0a31b715p+0, 0x1.6f46ad23182e4p-55 },
	{ 0x1.33c08b26416ffp+0, 0x1.32721843659a6p-54 },
	{ 0x1.371a7373aa9cbp+0, -0x1.63aeabf42eae2p-54 },
	{ 0x1.3a7db34e59ff7p+0, -0x1.5e436d661f5e3p-56 },
	{ 0x1.3dea64c123422p+0, 0x1.ada0911f09ebcp-55 },
	{ 0x1.4160a21f72e2ap+0, -0x1.ef3691c309278p-58 },
	{ 0x1.44e086061892dp+0, 0x1.89b7a04ef80d0p-59 },
	{ 0x1.486a2b5c13cd0p+0, 0x1.3c1a3b69062f0p-56 },
	{ 0x1.4bfdad5362a27p+0, 0x1.d4397afec42e2p-56 },
	{ 0x1.4f9b2769d2ca7p+0, -0x1.4b309d25957e3p-54 },
	{ 0x1.5342b569d4f82p+0, -0x1.07abe1db13cadp-55 },
	{ 0x1.56f4736b527dap+0, 0x1.9bb2c011d93adp-54 },
	{ 0x1.5ab07dd485429p+0, 0x1.6324c054647adp-54 },
	{ 0x1.5e76f15ad2148p+0, 0x1.ba6f93080e65ep-54 },
	{ 0x1.6247eb03a5585p+0, -0x1.383c17e40b497p-54 },
	{ 0x1.6623882552225p+0, -0x1.bb60987591c34p-54 },
	{ 0x1.6a09e667f3bcdp+0, -0x1.bdd3413b26456p-54 },
	{ 0x1.6dfb23c651a2fp+0, -0x1.bbe3a683c88abp-57 },
	{ 0x1.71f75e8ec5f74p+0, -0x1.16e4786887a99p-55 },
	{ 0x1.75feb564267c9p+0, -0x1.0245957316dd3p-54 },
	{ 0x1.7a11473eb0187p+0, -0x1.41577ee04992fp-55 },
	{ 0x1.7e2f336cf4e62p+0, 0x1.05d02ba15797ep-56 },
	{ 0x1.82589994cce13p+0, -0x1.d4c1dd41532d8p-54 },
	{ 0x1.868d99b4492edp+0, -0x1.fc6f89bd4f6bap-54 },
	{ 0x1.8ace5422aa0dbp+0, 0x1.6e9f156864b27p-54 },
	{ 0x1.8f1ae99157736p+0, 0x1.5cc13a2e3976cp-55 },
	{ 0x1.93737b0cdc5e5p+0, -0x1.75fc781b57ebcp-57 },
	{ 0x1.97d829fde4e50p+0, -0x1.d185b7c1b85d1p-54 },
	{ 0x1.9c49182a3f090p+0, 0x1.c7c46b071f2bep-56 },
	{ 0x1.a0c667b5de565p+0, -0x1.359495d1cd533p-54 },
	{ 0x1.a5503b23e255dp+0, -0x1.d2f6edb8d41e1p-54 },
	{ 0x1.a9e6b5579fdbfp+0, 0x1.0fac90ef7fd31p-54 },
	{ 0x1.ae89f995ad3adp+0, 0x1.7a1cd345dcc81p-54 },
	{ 0x1.b33a2b84f15fbp+0, -0x1.2805e3084d708p-57 },
	{ 0x1.b7f76f2fb5e47p+0, -0x1.5584f7e54ac3bp-56 },
	{ 0x1.bcc1e904bc1d2p+0, 0x1.23dd07a2d9e84p-55 },
	{ 0x1.c199bdd85529cp+0, 0x1.11065895048ddp-55 },
	{ 0x1.c67f12e57d14bp+0, 0x1.2884dff483cadp-54 },
	{ 0x1.cb720dcef9069p+0, 0x1.503cbd1e949dbp-56 },
	{ 0x1.d072d4a07897cp+0, -0x1.cbc3743797a9cp-54 },
	{ 0x1.d5818dcfba487p+0, 0x1.2ed02d75b3707p-55 },
	{ 0x1.da9e603db3285p+0, 0x1.c2300696db532p-54 },
	{ 0x1.dfc97337b9b5fp+0, -0x1.1a5cd4f184b5cp-54 },
	{ 0x1.e502ee78b3ff6p+0, 0x1.39e8980a9cc8fp-55 },
	{ 0x1.ea4afa2a490dap+0, -0x1.e9c23179c2893p-54 },
	{ 0x1.efa1bee615a27p+0, 0x1.dc7f486a4b6b0p-54 },
	{ 0x1.f50765b6e4540p+0, 0x1.9d3e12dd8a18bp-54 },
	{ 0x1.fa7c1819e90d8p+0, 0x1.74853f3a5931ep-55 },
};

/*
 * Tang's method (docs/research/libm-exp-log.md): x = n ln2/64 + r with
 * |r| <= ln2/128 (rounding to nearest), n = 64k + j, so exp(x) =
 * 2^k * 2^(j/64) * exp(r). ln2/64 is split so that n times its 32-bit
 * high part is exact for |n| below 2^21; r is kept as a double-double.
 * exp(r) - 1 is the Taylor series to r^7, whose remainder is below 2^-67.
 */
static struct __libc_dd nearest(double xh, double xl, int *exponent)
{
	const double inverse = 0x1.71547652b82fep+6;
	const double ln2_64_high = 0x1.62e42ff000000p-7;
	const double ln2_64_low = -0x1.718432a1b0e26p-41;
	double count = floor(xh * inverse + 0.5);
	int n = (int)count;
	int j = n & 63;
	struct __libc_dd r = __libc_dd_sum(xh - count * ln2_64_high, -count * ln2_64_low);
	double rh = r.hi;
	double rl = r.lo + xl;
	double polynomial = rh * rh * (0.5 + rh * (0x1.5555555555555p-3 + rh * (0x1.5555555555555p-5 + rh * (0x1.1111111111111p-7 + rh * (0x1.6c16c16c16c17p-10 + rh * 0x1.a01a01a01a01ap-13)))));
	struct __libc_dd t = table[j];
	struct __libc_dd product = __libc_dd_product(t.hi, rh);
	double low = t.lo + product.lo + t.hi * (rl + polynomial + rh * rl) + t.lo * rh;
	struct __libc_dd sum = __libc_dd_fast_sum(t.hi, product.hi);

	*exponent = (n - j) / 64;
	return __libc_dd_fast_sum(sum.hi, sum.lo + low);
}

/* Evaluated rounding to nearest, where the double-double steps are exact */
struct __libc_dd __libc_exp_core(double xh, double xl, int *exponent)
{
	int mode = __libc_math_nearest();
	struct __libc_dd result = nearest(xh, xl, exponent);

	__libc_math_restore(mode);
	return result;
}
