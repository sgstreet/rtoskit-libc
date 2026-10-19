/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_log_core.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* log(1 + j/128), j = -38..53, as double-double (computed to 70 digits) */
static const struct __libc_dd table[92] = {
	{ -0x1.68ac83e9c6a14p-2, -0x1.a64eadd740178p-58 },
	{ -0x1.5d5bddf595f30p-2, 0x1.6541148cbb8a2p-56 },
	{ -0x1.522ae0738a3d8p-2, 0x1.8f7e9b38a6979p-57 },
	{ -0x1.4718dc271c41bp-2, -0x1.8fb4c14c56eefp-60 },
	{ -0x1.3c25277333184p-2, 0x1.2ad27e50a8ec6p-56 },
	{ -0x1.314f1e1d35ce4p-2, 0x1.3d69909e5c3dcp-56 },
	{ -0x1.269621134db92p-2, -0x1.e0efadd9db02bp-56 },
	{ -0x1.1bf99635a6b95p-2, 0x1.12aeb84249223p-57 },
	{ -0x1.1178e8227e47cp-2, 0x1.0e63a5f01c691p-57 },
	{ -0x1.07138604d5862p-2, -0x1.cdb16ed4e9138p-56 },
	{ -0x1.f991c6cb3b379p-3, -0x1.f665066f980a2p-57 },
	{ -0x1.e530effe71012p-3, -0x1.2276041f43042p-59 },
	{ -0x1.d1037f2655e7bp-3, -0x1.60629242471a2p-57 },
	{ -0x1.bd087383bd8adp-3, -0x1.dd355f6a516d7p-60 },
	{ -0x1.a93ed3c8ad9e3p-3, -0x1.bcafa9de97203p-57 },
	{ -0x1.95a5adcf7017fp-3, -0x1.142c507fb7a3dp-58 },
	{ -0x1.823c16551a3c2p-3, 0x1.1232ce70be781p-57 },
	{ -0x1.6f0128b756abcp-3, 0x1.8de59c21e166cp-57 },
	{ -0x1.5bf406b543db2p-3, 0x1.1f5b44c0df7e7p-61 },
	{ -0x1.4913d8333b561p-3, 0x1.0d5604930f135p-58 },
	{ -0x1.365fcb0159016p-3, -0x1.7d411a5b944adp-58 },
	{ -0x1.23d712a49c202p-3, 0x1.6e38161051d69p-57 },
	{ -0x1.1178e8227e47cp-3, 0x1.0e63a5f01c691p-58 },
	{ -0x1.fe89139dbd566p-4, 0x1.ac9f4215f9393p-58 },
	{ -0x1.da727638446a2p-4, -0x1.401fa71733019p-58 },
	{ -0x1.b6ac88dad5b1cp-4, 0x1.0057eed1ca59fp-59 },
	{ -0x1.9335e5d594989p-4, 0x1.478a85704ccb7p-58 },
	{ -0x1.700d30aeac0e1p-4, 0x1.72566212cdd05p-61 },
	{ -0x1.4d3115d207eacp-4, -0x1.769f42c7842ccp-58 },
	{ -0x1.2aa04a44717a5p-4, 0x1.d15d38d2fa3f7p-58 },
	{ -0x1.08598b59e3a07p-4, 0x1.dd7009902bf32p-58 },
	{ -0x1.ccb73cdddb2ccp-5, 0x1.e48fb0500efd4p-59 },
	{ -0x1.894aa149fb343p-5, -0x1.a8be97660a23dp-60 },
	{ -0x1.466aed42de3eap-5, 0x1.cdd6f7f4a137ep-59 },
	{ -0x1.0415d89e74444p-5, -0x1.c05cf1d753622p-59 },
	{ -0x1.8492528c8cabfp-6, 0x1.d192d0619fa67p-60 },
	{ -0x1.0205658935847p-6, -0x1.27c8e8416e71fp-60 },
	{ -0x1.010157588de71p-7, -0x1.46662d417ced0p-62 },
	{ 0x0.0p+0, 0x0.0p+0 },
	{ 0x1.fe02a6b106789p-8, -0x1.e44b7e3711ebfp-67 },
	{ 0x1.fc0a8b0fc03e4p-7, -0x1.83092c59642a1p-62 },
	{ 0x1.7b91b07d5b11bp-6, -0x1.5b602ace3a510p-60 },
	{ 0x1.f829b0e783300p-6, 0x1.33e3f04f1ef23p-60 },
	{ 0x1.39e87b9febd60p-5, -0x1.5bfa937f551bbp-59 },
	{ 0x1.77458f632dcfcp-5, 0x1.18d3ca87b9296p-59 },
	{ 0x1.b42dd711971bfp-5, -0x1.eb9759c130499p-60 },
	{ 0x1.f0a30c01162a6p-5, 0x1.85f325c5bbacdp-59 },
	{ 0x1.16536eea37ae1p-4, -0x1.79da3e8c22cdap-60 },
	{ 0x1.341d7961bd1d1p-4, -0x1.b599f227becbbp-58 },
	{ 0x1.51b073f06183fp-4, 0x1.a49e39a1a8be4p-58 },
	{ 0x1.6f0d28ae56b4cp-4, -0x1.906d99184b992p-58 },
	{ 0x1.8c345d6319b21p-4, -0x1.4a697ab3424a9p-61 },
	{ 0x1.a926d3a4ad563p-4, 0x1.942f48aa70ea9p-58 },
	{ 0x1.c5e548f5bc743p-4, 0x1.5d617ef8161b1p-60 },
	{ 0x1.e27076e2af2e6p-4, -0x1.61578001e0162p-60 },
	{ 0x1.fec9131dbeabbp-4, -0x1.5746b9981b36cp-58 },
	{ 0x1.0d77e7cd08e59p-3, 0x1.9a5dc5e9030acp-57 },
	{ 0x1.1b72ad52f67a0p-3, 0x1.483023472cd74p-58 },
	{ 0x1.29552f81ff523p-3, 0x1.301771c407dbfp-57 },
	{ 0x1.371fc201e8f74p-3, 0x1.de6cb62af18a0p-58 },
	{ 0x1.44d2b6ccb7d1ep-3, 0x1.9f4f6543e1f88p-57 },
	{ 0x1.526e5e3a1b438p-3, -0x1.746ff8a470d3ap-57 },
	{ 0x1.5ff3070a793d4p-3, -0x1.bc60efafc6f6ep-58 },
	{ 0x1.6d60fe719d21dp-3, -0x1.caae268ecd179p-57 },
	{ 0x1.7ab890210d909p-3, 0x1.be36b2d6a0608p-59 },
	{ 0x1.87fa06520c911p-3, -0x1.bf7fdbfa08d9ap-57 },
	{ 0x1.9525a9cf456b4p-3, 0x1.d904c1d4e2e26p-57 },
	{ 0x1.a23bc1fe2b563p-3, 0x1.93711b07a998cp-59 },
	{ 0x1.af3c94e80bff3p-3, -0x1.398cff3641985p-58 },
	{ 0x1.bc286742d8cd6p-3, 0x1.4fce744870f55p-58 },
	{ 0x1.c8ff7c79a9a22p-3, -0x1.4f689f8434012p-57 },
	{ 0x1.d5c216b4fbb91p-3, 0x1.6e443597e4d40p-57 },
	{ 0x1.e27076e2af2e6p-3, -0x1.61578001e0162p-59 },
	{ 0x1.ef0adcbdc5936p-3, 0x1.48637950dc20dp-57 },
	{ 0x1.fb9186d5e3e2bp-3, -0x1.caaae64f21acbp-57 },
	{ 0x1.0402594b4d041p-2, -0x1.28ec217a5022dp-57 },
	{ 0x1.0a324e27390e3p-2, 0x1.7dcfde8061c03p-56 },
	{ 0x1.1058bf9ae4ad5p-2, 0x1.89fa0ab4cb31dp-58 },
	{ 0x1.1675cababa60ep-2, 0x1.ce63eab883717p-61 },
	{ 0x1.1c898c16999fbp-2, -0x1.0e5c62aff1c44p-60 },
	{ 0x1.22941fbcf7966p-2, -0x1.76f5eb09628afp-56 },
	{ 0x1.2895a13de86a3p-2, 0x1.7ad24c13f040ep-56 },
	{ 0x1.2e8e2bae11d31p-2, -0x1.8f4cdb95ebdf9p-56 },
	{ 0x1.347dd9a987d55p-2, -0x1.4dd4c580919f8p-57 },
	{ 0x1.3a64c556945eap-2, -0x1.c68651945f97cp-57 },
	{ 0x1.404308686a7e4p-2, -0x1.0bcfb6082ce6dp-56 },
	{ 0x1.4618bc21c5ec2p-2, 0x1.f42decdeccf1dp-56 },
	{ 0x1.4be5f957778a1p-2, -0x1.259b35b04813dp-57 },
	{ 0x1.51aad872df82dp-2, 0x1.3927ac19f55e3p-59 },
	{ 0x1.5767717455a6cp-2, 0x1.526adb283660cp-56 },
	{ 0x1.5d1bdbf5809cap-2, 0x1.4236383dc7fe1p-56 },
	{ 0x1.62c82f2b9c795p-2, 0x1.7b7af915300e5p-57 },
};

/*
 * Tang's method (docs/research/libm-exp-log.md): x = 2^e * m with m in
 * [sqrt(2)/2, sqrt(2)], c = 1 + j/128 the nearest grid point, f = (m - c)/c
 * as a double-double (m - c is exact), and log(x) = e ln2 + log(c) +
 * log1p(f) with |f| < 2^-7.5. log1p(f) is the series to f^9; its f^2/2 term
 * is exact. Near 1 (e = 0, c = 1) the result is relatively accurate.
 */
static struct __libc_dd nearest(double x)
{
	const double ln2_high = 0x1.62e42fefa3800p-1;
	const double ln2_low = 0x1.ef35793c76730p-45;
	uint64_t bits = __libc_double_bits(x);
	int e = __libc_double_exponent(bits);
	double m;
	int j;
	double c;
	double d;
	double fh;
	double fl;
	struct __libc_dd q;
	struct __libc_dd square;
	struct __libc_dd head;
	struct __libc_dd log1p;
	struct __libc_dd sum;
	struct __libc_dd total;
	double cubic;

	if (e == 0) {
		bits = __libc_double_bits(x * 0x1p54);
		e = __libc_double_exponent(bits) - 54;
	}
	e -= __LIBC_DOUBLE_BIAS;
	m = __libc_double_from((bits & __LIBC_DOUBLE_FRACTION) | ((uint64_t)__LIBC_DOUBLE_BIAS << 52));
	if (m > 0x1.6a09e667f3bcdp+0) {
		m *= 0.5;
		++e;
	}

	j = (int)((m - 1.0) * 128.0 + 128.5) - 128;
	c = 1.0 + j / 128.0;
	d = m - c;
	fh = d / c;
	q = __libc_dd_product(fh, c);
	fl = ((d - q.hi) - q.lo) / c;

	square = __libc_dd_product(fh, fh);
	cubic = fh * square.hi * (0x1.5555555555555p-2 + fh * (-0.25 + fh * (0.2 + fh * (-0x1.5555555555555p-3 + fh * (0x1.2492492492492p-3 + fh * (-0.125 + fh * 0x1.c71c71c71c71cp-4))))));
	head = __libc_dd_sum(fh, -0.5 * square.hi);
	log1p = __libc_dd_fast_sum(head.hi, head.lo + fl - 0.5 * square.lo - fh * fl + cubic);

	sum = __libc_dd_sum(e * ln2_high, table[j + 38].hi);
	total = __libc_dd_sum(sum.hi, log1p.hi);
	return __libc_dd_fast_sum(total.hi, total.lo + sum.lo + table[j + 38].lo + e * ln2_low + log1p.lo);
}

/* Evaluated rounding to nearest, where the double-double steps are exact */
struct __libc_dd __libc_log_core(double x)
{
	int mode = __libc_math_nearest();
	struct __libc_dd result = nearest(x);

	__libc_math_restore(mode);
	return result;
}
