/*
 * Copyright (C) 2026 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * near.inc
 *
 * Created on: Oct 21, 2026
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

/*
 * Arguments whose sin, cos or tan lies so near the point halfway between
 * two floats that the Armv6-M fixed-point fast path (PLAN D87) rounds it the
 * wrong way without its error test (found over every binade on the host),
 * with the correctly rounded result (mpmath, 200 bits). Two tanf arguments
 * found (0x1.3240bcp-3, 0x1.b79bf6p+18) are left out: the float-float
 * quotient (D60) that every float-float board uses misses them as well, within the
 * allowance of D57.
 */
	{ sinf, 0x1.d3113p+24f, -0x1.6dcd46p-3f },
	{ cosf, 0x1.42bf9cp+10f, -0x1.f5b972p-1f },
	{ cosf, 0x1.50370cp+2f, 0x1.07a9f2p-1f },
	{ sinf, 0x1.4775ccp-1f, 0x1.319674p-1f },
	{ cosf, 0x1.c1fdd2p+8f, -0x1.78cdaep-1f },
	{ cosf, 0x1.1bc91ep+8f, 0x1.022df4p-1f },
	{ cosf, 0x1.dbfdb6p+11f, 0x1.e64d68p-1f },
	{ cosf, 0x1.8c1ea4p+24f, 0x1.306f46p-4f },
	{ sinf, 0x1.b8a7fap+26f, -0x1.cee35cp-1f },
	{ sinf, 0x1.ff5d1ap+25f, 0x1.fb1cd8p-1f },
	{ cosf, 0x1.412a82p+0f, 0x1.3e75e6p-2f },
	{ sinf, 0x1.a36f2cp+9f, -0x1.0311eep-4f },
	{ cosf, 0x1.9b1b72p+27f, 0x1.2c68d2p-2f },
	{ cosf, 0x1.465576p+21f, 0x1.ff7794p-2f },
	{ cosf, 0x1.c59074p-5f, 0x1.ff3728p-1f },
	{ sinf, 0x1.fdcc06p+2f, 0x1.fcd0bp-1f },
	{ sinf, 0x1.c7390cp+16f, 0x1.4e58e8p-2f },
	{ cosf, 0x1.3d9938p+7f, -0x1.2fac28p-3f },
	{ cosf, 0x1.bbfb94p+0f, -0x1.4d617ap-3f },
	{ sinf, 0x1.8a091p+13f, -0x1.e0d618p-1f },
	{ cosf, 0x1.f41d94p+26f, 0x1.919c8ep-2f },
	{ sinf, 0x1.3606f8p+0f, 0x1.df394cp-1f },
	{ cosf, 0x1.f07c04p+3f, -0x1.f682d4p-1f },
	{ sinf, 0x1.7eee04p+1f, 0x1.31f4c6p-3f },
	{ sinf, 0x1.f563bcp+28f, -0x1.81d15p-2f },
	{ sinf, 0x1.973bd6p-7f, 0x1.973928p-7f },
	{ sinf, 0x1.13079ap+30f, -0x1.fea94p-1f },
	{ cosf, 0x1.6ef82p+17f, -0x1.1c6a62p-1f },
	{ sinf, 0x1.6049d2p+8f, 0x1.aadc3ap-2f },
	{ cosf, 0x1.8968c2p+22f, -0x1.23c11cp-3f },
	{ sinf, 0x1.d12cep-12f, 0x1.d12cep-12f },
	{ sinf, 0x1.6c9676p+30f, 0x1.cda8fp-1f },
	{ tanf, 0x1.a52c6ep-9f, 0x1.a52ccep-9f },
	{ tanf, 0x1.24ad9p+7f, -0x1.eb2c26p+1f },
	{ tanf, 0x1.862ee2p+6f, 0x1.42f826p-3f },
	{ tanf, 0x1.0296dap+20f, -0x1.c2c9f2p-5f },
	{ tanf, 0x1.14562ap+7f, -0x1.facf16p-5f },
	{ tanf, 0x1.8b524cp-7f, 0x1.8b5736p-7f },
	{ tanf, 0x1.a5e26ap+29f, 0x1.0dd5d8p+1f },
	{ tanf, 0x1.cb6becp-7f, 0x1.cb73a2p-7f },
	{ tanf, 0x1.5445c6p+13f, -0x1.3b3356p-5f },
	{ tanf, 0x1.9cd92ep+5f, 0x1.110c86p+2f },
	{ tanf, 0x1.33d6bap+10f, -0x1.33e2e6p-3f },
	{ tanf, 0x1.be8826p-5f, 0x1.bef98p-5f },
	{ tanf, 0x1.178048p+7f, 0x1.3dac3ap+4f },
	{ tanf, 0x1.c6037p-9f, 0x1.c603e8p-9f },
