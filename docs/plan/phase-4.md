# Phase 4 — crosstool-NG multilib GCC cross-toolchain integration

Prerequisite: `phase-3-complete`. The groundwork is already in earlier phases:
D15 (one header set per target triple) and D16 (a build that can be driven from
outside) must hold from Phase 1 on.

**Goal:** crosstool-NG builds a multilib `arm-none-eabi` GCC whose C library is
rtoskit-libc. A user can then run `arm-none-eabi-gcc -mcpu=cortex-m33 ... foo.c`
and get the right libc variant, crt and RTOS port library with no extra flags
(or with one documented `--specs=`).

Open questions that shape this phase: **Q8** (C++ / `libstdc++`) and **Q9**
(where the integration lives; which multilib set). See PLAN.md.

**Every step's definition of done (DoD)** is the Phase 1 DoD plus:

- libc-test passes for **every multilib** of the produced toolchain (run under
  `qemu-arm` against `librtos-linux.a`).
- The libc objects built by the toolchain flow match the in-tree build for the
  same arch and flags (identical `size` output per member).

Note: a full crosstool-NG build takes 30–90 minutes and several GB of disk.
Check the session's disk allowance first, and keep `ct-ng` build directories
outside the repo.

---

## P4.01 — Research and integration design
- [ ] Research how other C libraries plug into crosstool-NG: the libc backends (`scripts/build/libc/{newlib,picolibc,musl}.sh`, `config/libc/*.in`), the companion-libs mechanism, and the build order (`libc_headers` → core gcc → `libc` per multilib → final gcc).
- [ ] Research GCC multilib for `arm-none-eabi`: `--with-multilib-list=rmprofile` vs a custom `t-*` multilib file, `-print-multi-lib`/`-print-multi-directory`, `MULTILIB_REUSE`, and how picolibc and newlib install per multilib and ship `.specs` files.
- [ ] Decide (record in PLAN.md):
  - Integration form: a new crosstool-NG libc backend `rtoskit`, kept as a patch series in `toolchain/crosstool-ng/`, vs a fork vs `libc=none` with a post-step (Q9).
  - The multilib list (Q9).
  - Default port library linkage: `librtos-none.a` by default and `--specs=rtoskit-linux.specs` for the test port, vs explicit `-lrtos-<port>`.
  - C++ (Q8).
- [ ] Write the research note `docs/research/crosstool-ng-integration.md`.

## P4.02 — Make the libc build multilib-ready (in-tree)
- [ ] Generic multilib toolchain mode: `tools/makefiles/arm-none-eabi.mk`, where the arch flags come from `MULTILIB_FLAGS` and `MULTIDIR` (D16). The existing `armv6-m`/`armv7e-m`/`armv7e-m-dp`/`armv8-m.main` files become thin presets over it.
- [ ] Map each multilib to a board/`ARCH_CROSS` toolchain file and its variant subdirectories (D22) (e.g. the v7e-m soft-float multilib reuses the v7e-m code with the soft-float `fenv`, and the `thumb/v7e-m+dp/hard` multilib maps to the `armv7e-m-dp` layers). Multilibs outside the optimization matrix (if the list includes them) fall back to generic C, plus the minimal arch files they need (`setjmp`, `fenv`, crt).
- [ ] `install` target: headers to `${PREFIX}/include`; and `libc.a`, `libm.a`, `crt0.o`, `librtos-none.a`, `librtos-linux.a` and the `.specs` files to `${PREFIX}/lib/${MULTIDIR}`.
- [ ] `install-headers` target (headers only, no compiler needed beyond the preprocessor).
- [ ] Header audit for D15: one header set compiles and gives identical `sizeof`/`alignof` for every type across all multilib flag sets (an automated test).
- [ ] `make multilib CROSS_COMPILE=…`: iterate `$(CC) -print-multi-lib`, then build and install every variant. Run the libc-test gate per variant.

## P4.03 — Bootstrap compatibility
- [ ] Build libc with a **stage-1 GCC** (configured `--without-headers`, no libc, `inhibit_libc`). Confirm that nothing needs libgcc headers or a prebuilt sysroot.
- [ ] Check what libgcc needs from the libc headers when GCC is configured with our sysroot (`--with-sysroot`, `--with-native-system-header-dir=/include`). Fix any gaps (for example `<stdint.h>` via `--enable-stdint=provide`/`wrap`, `<limits.h>` include_next interplay with `fixincludes`; consider `--disable-fixincludes` or a no-op fixinc).
- [ ] TLS: check that `--enable-tls` and the `__aeabi_read_tp` contract work with GCC's ARM EABI TLS model on M-profile (D10).

## P4.04 — crosstool-NG backend
- [ ] `toolchain/crosstool-ng/`: the libc backend (`config/libc/rtoskit.in`, `scripts/build/libc/rtoskit.sh` with `do_rtoskit_get/extract/headers/build/post_cc`), with the source taken from this repo's git URL at a tag or commit, or from a local directory for development.
- [ ] Patch series for a pinned crosstool-NG release, plus an `apply` script. Also upstreamable as a stand-alone PR if desired.
- [ ] Sample config `toolchain/crosstool-ng/samples/arm-none-eabi-rtoskit/crosstool.config`: GCC version (≥ 14, preferably matching the 15.2 in `setup.mk`), binutils, the multilib list, `--enable-languages=c` (or C/C++ per Q8), and `--disable-nls`.
- [ ] `make toolchain` target that drives `ct-ng` (download, apply the patches, build) into `local/` or `tmp/`, consistent with `setup.mk` conventions.

## P4.05 — GCC driver integration
- [ ] `.specs` files: the default `startfile` (`crt0.o`), `lib` (`-lc -lrtos-none -lgcc`), and `rtoskit-linux.specs` for the test port. Check `-nostartfiles`/`-nodefaultlibs` behaviour.
- [ ] The multilib directory layout matches `gcc -print-multi-directory` for every supported `-mcpu`/`-march`/`-mfloat-abi` combination, including `MULTILIB_REUSE` fallbacks.
- [ ] If Q8 = C++: build `libstdc++` against rtoskit-libc (the configure flags for the missing wide-character support), and run a C++ smoke test.

## P4.06 — End-to-end validation
- [ ] Build the full toolchain with crosstool-NG from a clean state.
- [ ] Build libc-test with the **installed toolchain** (no in-tree include or lib paths), for every multilib. Run it under `qemu-arm` and compare with the in-tree baselines (results must be identical).
- [ ] Object equivalence check: toolchain-built vs in-tree-built archives (`size` per member).
- [ ] Rebuild `example/hello-world` and the `test/size` reference programs with the installed toolchain; the sizes must match the Phase 2/3 budgets.

## P4.07 — Packaging and wrap-up
- [ ] Release tarball of the toolchain (naming as in `setup.mk`'s `COMPILER_ARCHIVE` convention). Wire `tools/makefiles/setup.mk` (`COMPILER_URL`/`COMPILER_TARGET`) so `make setup` can fetch it.
- [ ] `docs/toolchain.md`: how to build the toolchain, use it, choose a port library, and add a multilib.
- [ ] Tag `phase-4-complete`.
