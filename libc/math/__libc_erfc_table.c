/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * __libc_erfc_table.c
 *
 * Created on: Oct 19, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <math.h>
#include <stdint.h>

#include "internal/math.h"

/* erfc(c) and 2/sqrt(pi) exp(-c^2) for c = 1/2 + k/16, k = 0 ... 56, as double-doubles */
static const struct {
	struct __libc_dd erfc;
	struct __libc_dd slope;
} table[] = {
	{ { .hi = 0x1.eb02147ce245cp-2, .lo = -0x1.5e809f1a31a28p-56 }, { .hi = 0x1.c1efca49a5011p-1, .lo = 0x1.4c081d7f49500p-55 } },
	{ { .hi = 0x1.b48eaee924501p-2, .lo = 0x1.ab2d71290b7aap-57 }, { .hi = 0x1.a5074e2157620p-1, .lo = 0x1.fad8c0ef6faecp-56 } },
	{ { .hi = 0x1.81cd2465e1d96p-2, .lo = 0x1.f25f4f6fdf70bp-56 }, { .hi = 0x1.86e9694134b9ep-1, .lo = -0x1.3bda1314b1d68p-55 } },
	{ { .hi = 0x1.52db785a98acap-2, .lo = -0x1.8eef7012e8df5p-56 }, { .hi = 0x1.681ff24b4ab04p-1, .lo = -0x1.dba6493354c66p-58 } },
	{ { .hi = 0x1.27c6d14c5e341p-2, .lo = 0x1.3af3434d0eeabp-57 }, { .hi = 0x1.492e42d78d2c5p-1, .lo = -0x1.8bd386a7ba3f6p-55 } },
	{ { .hi = 0x1.008c80a24ff10p-2, .lo = -0x1.66cf14bcad032p-56 }, { .hi = 0x1.2a8dcede3673bp-1, .lo = -0x1.7378e2c70325dp-56 } },
	{ { .hi = 0x1.ba36dab91c0e9p-3, .lo = 0x1.3c896e9a97c59p-58 }, { .hi = 0x1.0cab61f084b93p-1, .lo = 0x1.098a511a778e7p-56 } },
	{ { .hi = 0x1.7aab97a554544p-3, .lo = -0x1.a558a46df5f67p-57 }, { .hi = 0x1.dfca26f5bbf88p-2, .lo = -0x1.ddcbaf85587b9p-57 } },
	{ { .hi = 0x1.4226162fbddd5p-3, .lo = -0x1.b40443f6ec34ap-59 }, { .hi = 0x1.a911f096fbc26p-2, .lo = -0x1.086a09f735b33p-56 } },
	{ { .hi = 0x1.1043c1086777dp-3, .lo = 0x1.91608a4706a42p-58 }, { .hi = 0x1.75a91a7f4d2edp-2, .lo = 0x1.6eb826a9df857p-58 } },
	{ { .hi = 0x1.c9296beb09cf1p-4, .lo = -0x1.5224acd170beep-59 }, { .hi = 0x1.45e99bcbb7915p-2, .lo = 0x1.7bcd0125a8155p-56 } },
	{ { .hi = 0x1.7d3fa69816db5p-4, .lo = 0x1.81ace739008b7p-58 }, { .hi = 0x1.1a0dc51a9934dp-2, .lo = -0x1.ca89d2d78fba4p-57 } },
	{ { .hi = 0x1.3bcd133aa0ffcp-4, .lo = -0x1.89da82345938bp-62 }, { .hi = 0x1.e4652fadcb6b2p-3, .lo = -0x1.e950836901024p-61 } },
	{ { .hi = 0x1.03d0ab9273b94p-4, .lo = -0x1.1c2fb565dcadep-60 }, { .hi = 0x1.9cb5bd549b111p-3, .lo = -0x1.973e73caa1edep-59 } },
	{ { .hi = 0x1.a8973c4b5c03ep-5, .lo = 0x1.d27662c1d9dc2p-59 }, { .hi = 0x1.5ce595c455b0ap-3, .lo = 0x1.c317415c2c6e1p-59 } },
	{ { .hi = 0x1.588cf12f4446bp-5, .lo = 0x1.3a42831ec7755p-59 }, { .hi = 0x1.24a7b84d38971p-3, .lo = 0x1.aa0c5e788ed5fp-57 } },
	{ { .hi = 0x1.15aaa8ec85205p-5, .lo = -0x1.e86ee834da4cep-61 }, { .hi = 0x1.e723726b824a9p-4, .lo = -0x1.2203197eea764p-59 } },
	{ { .hi = 0x1.bc6c1da1f3121p-6, .lo = 0x1.fbf90a59c8999p-60 }, { .hi = 0x1.92470a61b6965p-4, .lo = 0x1.c6acd40cee350p-58 } },
	{ { .hi = 0x1.612d893085125p-6, .lo = -0x1.7847afe4f2a7bp-62 }, { .hi = 0x1.499d478bca735p-4, .lo = 0x1.31c41d17378e2p-60 } },
	{ { .hi = 0x1.16b24cb8f8f92p-6, .lo = -0x1.9d194a11ca30dp-62 }, { .hi = 0x1.0bf97e95f2a64p-4, .lo = -0x1.446051f6fef83p-58 } },
	{ { .hi = 0x1.b4be201caa4b4p-7, .lo = -0x1.6abde927f9cddp-61 }, { .hi = 0x1.b055303221015p-5, .lo = 0x1.cba9ea60ed019p-59 } },
	{ { .hi = 0x1.53c89d8bb3ddbp-7, .lo = -0x1.9e42624e454bfp-63 }, { .hi = 0x1.5a08e85af27e0p-5, .lo = 0x1.e4f9cfc8c2382p-59 } },
	{ { .hi = 0x1.0678442cc256fp-7, .lo = -0x1.77b62199d8601p-61 }, { .hi = 0x1.12ceb37ff9bc3p-5, .lo = 0x1.a3b0b09d34761p-59 } },
	{ { .hi = 0x1.9299afa0246a6p-8, .lo = -0x1.e613bc6b0debdp-62 }, { .hi = 0x1.b1160991ff737p-6, .lo = -0x1.d940a504353b9p-61 } },
	{ { .hi = 0x1.328f5ec350e67p-8, .lo = -0x1.ca006412e68d0p-62 }, { .hi = 0x1.529b9e8cf9a1ep-6, .lo = 0x1.b47becf12c4e4p-61 } },
	{ { .hi = 0x1.cf80d4afc3019p-9, .lo = -0x1.6aabfff1e020bp-63 }, { .hi = 0x1.06ae13b0d3255p-6, .lo = -0x1.88abd7f4be982p-60 } },
	{ { .hi = 0x1.5bde729a6b60fp-9, .lo = 0x1.999ec7becc5c7p-65 }, { .hi = 0x1.94624e78e0fafp-7, .lo = -0x1.41864737c78ffp-61 } },
	{ { .hi = 0x1.033197ec68c0ep-9, .lo = -0x1.b0615fb45c6ffp-63 }, { .hi = 0x1.34d7dbc76d7e5p-7, .lo = 0x1.3780d6e7eb350p-61 } },
	{ { .hi = 0x1.7f713f9cc9784p-10, .lo = -0x1.4207143202515p-64 }, { .hi = 0x1.d4143a9dfe965p-8, .lo = -0x1.6eec3c35c7ed7p-63 } },
	{ { .hi = 0x1.198fd6a0ee7bdp-10, .lo = 0x1.78dc7315b06e8p-64 }, { .hi = 0x1.5ff2750fe7820p-8, .lo = -0x1.5f184847ca665p-62 } },
	{ { .hi = 0x1.9a7c305336484p-11, .lo = 0x1.6394dd2ff0093p-65 }, { .hi = 0x1.06918b6355624p-8, .lo = 0x1.21b463b8e3a00p-62 } },
	{ { .hi = 0x1.29082600643fdp-11, .lo = 0x1.8be7e5c848c07p-65 }, { .hi = 0x1.84ba3004a50d0p-9, .lo = -0x1.90b93d46321efp-64 } },
	{ { .hi = 0x1.aab859b20ac9ep-12, .lo = 0x1.88f4ff748376bp-66 }, { .hi = 0x1.1d83170fbf6fbp-9, .lo = 0x1.ea3671efbb74ap-63 } },
	{ { .hi = 0x1.30439c56dadf6p-12, .lo = 0x1.2900671ba2e85p-67 }, { .hi = 0x1.a024365f771bdp-10, .lo = 0x1.3c8f5202cb3e9p-64 } },
	{ { .hi = 0x1.aeb4423e690e7p-13, .lo = -0x1.a732e928e4c5dp-68 }, { .hi = 0x1.2ce898809244ep-10, .lo = 0x1.092b45493a9e8p-64 } },
	{ { .hi = 0x1.2e984ed53e777p-13, .lo = 0x1.9c7db1f96e779p-72 }, { .hi = 0x1.afc85e0f82e12p-11, .lo = 0x1.438f22895e01dp-66 } },
	{ { .hi = 0x1.a609f7584d32bp-14, .lo = 0x1.d92f3f7ab9ef5p-68 }, { .hi = 0x1.3360ccd23db3ap-11, .lo = 0x1.39e15444e9dddp-69 } },
	{ { .hi = 0x1.2422ed95a3235p-14, .lo = -0x1.8174270a3a7eap-68 }, { .hi = 0x1.b23a5a23e4210p-12, .lo = 0x1.727bce1bdfffep-67 } },
	{ { .hi = 0x1.916f7c5f2f764p-15, .lo = 0x1.84e95673c70a1p-70 }, { .hi = 0x1.30538fbb77ecdp-12, .lo = 0x1.8c5e8c1b3532fp-69 } },
	{ { .hi = 0x1.11c3bed8e716ap-15, .lo = -0x1.899ad47c9a36bp-70 }, { .hi = 0x1.a740684026555p-13, .lo = -0x1.7e24cc3ac5720p-69 } },
	{ { .hi = 0x1.729df6503422ap-16, .lo = 0x1.784ca4c429a15p-73 }, { .hi = 0x1.2408e9ba3327fp-13, .lo = -0x1.7e1d81587040cp-67 } },
	{ { .hi = 0x1.f1febf7a916aap-17, .lo = 0x1.b7c360a6e4aeap-72 }, { .hi = 0x1.8fdc1b2dcf7b9p-14, .lo = 0x1.7050f50b8f311p-71 } },
	{ { .hi = 0x1.4c144d984e1b8p-17, .lo = 0x1.2d1f7068bf3fbp-74 }, { .hi = 0x1.0f9e1b4dd36dfp-14, .lo = -0x1.3fedc80d9c7aep-71 } },
	{ { .hi = 0x1.b792bbc489b04p-18, .lo = -0x1.a99758b268f91p-73 }, { .hi = 0x1.6e2367dc27f95p-15, .lo = 0x1.d96e6f01510e8p-73 } },
	{ { .hi = 0x1.20c1303550f0ep-18, .lo = -0x1.20ee80d2c8d09p-73 }, { .hi = 0x1.e9b5e8d00ce77p-16, .lo = -0x1.cbbe2ecd1cfacp-70 } },
	{ { .hi = 0x1.7885ce9f67cdbp-19, .lo = -0x1.004a4c0a6b8a9p-73 }, { .hi = 0x1.44f21e49054f2p-16, .lo = 0x1.f338cf40864d8p-71 } },
	{ { .hi = 0x1.e749309831666p-20, .lo = -0x1.76172989e68aep-76 }, { .hi = 0x1.abe09e9144b5ep-17, .lo = 0x1.308300923fe4bp-71 } },
	{ { .hi = 0x1.38f2e86f38216p-20, .lo = -0x1.04a7333baf0fdp-74 }, { .hi = 0x1.1783ceac28910p-17, .lo = -0x1.7f19d8ee582e5p-71 } },
	{ { .hi = 0x1.8ef2a9a18d857p-21, .lo = -0x1.2d76dc03e80a5p-75 }, { .hi = 0x1.6a597219a93dap-18, .lo = -0x1.cbf8fbc2cd5cdp-72 } },
	{ { .hi = 0x1.f8c0c2e2ce8dep-22, .lo = 0x1.ac2e9d31bb2d7p-76 }, { .hi = 0x1.d21397ead99cbp-19, .lo = -0x1.6abd9c029cf89p-75 } },
	{ { .hi = 0x1.3ce784b411931p-22, .lo = -0x1.8c77157062626p-76 }, { .hi = 0x1.296a70f414053p-19, .lo = 0x1.036e65808a17dp-74 } },
	{ { .hi = 0x1.8aee4cd06ec1bp-23, .lo = -0x1.519d0e06052c4p-79 }, { .hi = 0x1.789fb715aae95p-20, .lo = -0x1.226d93bf88ff6p-80 } },
	{ { .hi = 0x1.e87470e4f4246p-24, .lo = -0x1.cafa3aa5b4314p-82 }, { .hi = 0x1.d9371e2ff7c35p-21, .lo = 0x1.0aed0951e078dp-75 } },
	{ { .hi = 0x1.2bc82ab9d2302p-24, .lo = 0x1.4b5a3c94128b3p-78 }, { .hi = 0x1.26f9df8519bd7p-21, .lo = -0x1.e339871c01768p-75 } },
	{ { .hi = 0x1.6d3126d74b6ccp-25, .lo = -0x1.301eb5249fe1dp-79 }, { .hi = 0x1.6ce1aa3fd7bddp-22, .lo = 0x1.bf15b0bc56aabp-80 } },
	{ { .hi = 0x1.b9823c51276e1p-26, .lo = -0x1.a2e44396d39b5p-80 }, { .hi = 0x1.bfd7555a3bd68p-23, .lo = 0x1.0151cf177a911p-77 } },
	{ { .hi = 0x1.08ddd13bd35e7p-26, .lo = -0x1.615db40319381p-80 }, { .hi = 0x1.10b1488aeb235p-23, .lo = -0x1.cd75b4828c0c0p-81 } },
};

/*
 * erfc(c + h) = erfc(c) - 2/sqrt(pi) exp(-c^2) S with
 * S = sum (-1)^(m-1) H_(m-1)(c) h^m / m! (the Taylor series of the integral
 * of exp(-t^2), Hermite polynomials by their recurrence), |h| <= 1/32; the
 * terms fall below 2^-60 of S by m = 15.
 */
struct __libc_dd __libc_erfc_table(double x)
{
	int k = (int)((x - 0.5) * 16.0 + 0.5);
	double c = 0.5 + k / 16.0;
	double h = x - c;
	double previous = 1.0;
	double hermite = 2.0 * c;
	double power = h;
	double sum = 0.0;
	struct __libc_dd s;

	for (int m = 2; m <= 15; ++m) {
		double next;

		power *= h / m;
		sum += ((m & 1) != 0 ? hermite : -hermite) * power;
		next = 2.0 * c * hermite - 2.0 * (m - 1) * previous;
		previous = hermite;
		hermite = next;
	}
	s = __libc_dd_fast_sum(h, sum);

	return __libc_dd_add(table[k].erfc, __libc_dd_negate(__libc_dd_multiply(table[k].slope, s)));
}
