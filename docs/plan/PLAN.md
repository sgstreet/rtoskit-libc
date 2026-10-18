# rtoskit-libc — Master Plan

This is the source of truth for the project's goals, architecture and decisions.
The work items live in the phase files:

- [phase-1.md](phase-1.md) — compliant implementation
- [phase-2.md](phase-2.md) — size optimization pass
- [phase-3.md](phase-3.md) — performance optimization pass
- [phase-4.md](phase-4.md) — crosstool-NG multilib GCC cross-toolchain integration

Status legend used in all phase files: `[ ]` todo, `[~]` in progress, `[x]` done,
`[-]` dropped (with a reason).

---

## 1. Goals

1. A clean-room, ISO/IEC 9899:2024 (C23, N3220) compliant C standard library for
   deeply embedded systems. The normative reference is `docs/standard/library-7.pdf`
   (clause 7) and `docs/standard/annex-b.pdf` (library summary).
2. Implemented in C23 (`-std=c23`). All per-thread library state uses
   `thread_local` (`errno`, `strtok`, `rand`, the `asctime`/`gmtime`/`localtime`
   static buffers, `tmpnam`, ...).
3. A single, explicit **RTOS seam** (`__rtos_*` functions) through which the
   library gets locks, threads, the TLS setup, time, I/O, the heap and process
   exit. No other OS dependency.
4. Minimal conditional compilation. Variation (arch, port, feature flavours) is
   chosen by the **build system selecting files** and by **link-time
   binding**, not by `#if`.
5. libc-test validates every step of every phase. No step is done until libc-test
   has no regressions against the recorded baseline on every enabled
   architecture.
6. Built only with the build system in `tools/`.

### Phases

| Phase | Goal | Exit criterion |
|---|---|---|
| 1 | Compliant implementation | Every applicable libc-test test passes on x86_64 and all ARM targets. The header audit against Annex B is clean. |
| 2 | Size pass | The size baseline is reduced per target, and libc-test still passes in full. |
| 3 | Performance pass | The benchmark baseline is improved per target, libc-test still passes in full, and the size budget from Phase 2 holds. |
| 4 | Toolchain integration | crosstool-NG builds a multilib `arm-none-eabi` GCC with rtoskit-libc as its C library. libc-test passes for every multilib when built with that toolchain, and its objects match the in-tree build. |

### Scope (decided)

**In scope (ISO C23 clause 7):** `<assert.h>` `<complex.h>` `<ctype.h>`
`<errno.h>` `<fenv.h>` `<float.h>` `<inttypes.h>` `<iso646.h>` `<limits.h>`
`<locale.h>` `<math.h>` `<setjmp.h>` `<signal.h>` `<stdalign.h>` `<stdarg.h>`
`<stdatomic.h>` `<stdbit.h>` `<stdbool.h>` `<stdckdint.h>` `<stddef.h>`
`<stdint.h>` `<stdio.h>` `<stdlib.h>` `<stdnoreturn.h>` `<string.h>`
`<tgmath.h>` `<threads.h>` `<time.h>`

**Out of scope:** `<wchar.h>`, `<wctype.h>`, `<uchar.h>`, POSIX extensions,
Annex K (bounds-checking interfaces), decimal floating point (`_Decimal*`, an
optional feature: `__STDC_IEC_60559_DFP__` is not defined).
The multibyte functions that `<stdlib.h>` itself declares (`mblen`, `mbtowc`,
`wctomb`, `mbstowcs`, `wcstombs`) are still required. They are implemented for
the "C" locale only (`MB_CUR_MAX == 1`), and `wchar_t` comes from `<stddef.h>`.

**Order:** libm is implemented **last** in Phase 1.

### Target matrix

| Target | `ARCH_CROSS` (D22) | Test board (`BOARD_TYPE`) | Core / ISA | `long double` | FPU | Role |
|---|---|---|---|---|---|---|
| `x86_64` | `x86_64` | `linux-x86_64` | x86-64 | 64-bit, see D7 | SSE2 | Phase 1 primary dev/test host and Phase 3 perf reference |
| `armv6-m` | `armv6-m` | `linux-armv6-m` | Cortex-M0+ (Thumb-1) | 64-bit | none (soft) | Phase 2 and 3 target |
| `armv7e-m` | `armv7e-m` | `linux-armv7e-m` | Cortex-M4F (Thumb-2, DSP) | 64-bit | FPv4-SP (hard) | Phase 2 and 3 target |
| `armv7e-m-dp` | `armv7e-m-dp` | `linux-armv7e-m-dp` | Cortex-M7 with the double-precision FPU (Thumb-2, DSP) | 64-bit | FPv5-D16, single + double (hard) | Phase 2 and 3 target |
| `armv8-m.main` | `armv8-m.main` | `linux-armv8-m.main` | Cortex-M33 (Thumb-2, DSP) | 64-bit | FPv5-SP (hard) | Phase 2 and 3 target |

The `linux-*` boards run the test binaries through `port/linux`, natively or
under `qemu-arm`. Real hardware boards (e.g. `pi-pico`, `pi-pico2`) with
`port/none` or `port/rtoskit` can be added as further `BOARD_TYPE` files.
The plan text uses the target names; build commands use the board and
`ARCH_CROSS` names.

RISC-V is not targeted for now. The layout allows it to be added as just
another `arch/` and toolchain file.

---

## 2. Decision log

Every decision gets an ID. Revisit a decision by adding a new entry that
supersedes it; do not edit history.

| ID | Decision | Rationale |
|---|---|---|
| D1 | **Clean-room implementation.** Other libcs (musl, picolibc, newlib, glibc, Bionic, FreeBSD, ARM optimized-routines, CORE-MATH, ...) are *studied* before writing any algorithm, but no code is copied or mechanically translated. | The user requires clean-room. Keeps the MPL-2.0 licensing unencumbered. |
| D2 | **Research before implementing.** Each non-trivial algorithm gets a note in `docs/research/<topic>.md`: the sources reviewed (with URLs), their trade-offs, the chosen approach, and its edge cases. Notes describe ideas in our own words and contain no copied code. | User requirement. It also records provenance as clean-room evidence. |
| D3 | **libc-test runs natively on a Linux "port".** `port/linux` implements the RTOS seam with raw Linux syscalls (no glibc). On x86_64 the test binaries run natively; ARM binaries run under `qemu-arm` (user mode). | It is fast, scriptable, and gives libc-test the process model it expects. |
| D4 | **The RTOS seam is link-time bound.** The library calls plain external functions `__rtos_*`, declared in `include/rtos/rtos-port.h` (an internal header installed for port authors). Each port supplies them in its own archive (`librtos-<port>.a`). There are no function-pointer tables and no `#ifdef RTOS_X`. Port-defined *types* (e.g. lock storage size) come from `port/<port>/include/rtos/rtos-port-types.h`, found through the include path. | It has zero runtime overhead, needs no conditional compilation, and keeps the seam explicit and auditable. |
| D5 | **Selection by files, not `#if`.** Arch overrides live in `arch/<arch>/src/<area>/<name>.{c,S}`. When an arch provides a file with the same stem as a generic `src/<area>/<name>.c`, the build uses the arch file instead. Arch-specific headers are found through `-I arch/<arch>/include` placed before `-I include`. | This is the user requirement to minimize conditional compilation. |
| D6 | **One external function per source file** (musl-style). Each `thread_local` state object is its own variable next to its only user, not a big shared TLS struct. | Archive members then pull in only what is used. With `-fdata-sections` unused `.tbss`/`.tdata` is dropped, so threads pay TLS only for features the image uses. It also makes arch overrides (D5) trivial. |
| D7 | *Proposed, needs confirmation:* build the x86_64 test configuration with **`-mlong-double-64`**, so `long double` matches the ARM targets (binary64). Everything, including libc-test, is compiled with the same flag and statically linked. | It avoids writing and maintaining a whole ld80 libm and printf/strtold path that no product target uses. libc-test's math tables select by `LDBL_MANT_DIG`, so the binary64 tables are used. If rejected, Phase 1 grows ld80 variants under `src/math/ld80/`. |
| D8 | **The library owns all headers**, including the freestanding ones (`<stddef.h>`, `<stdint.h>`, `<limits.h>`, `<float.h>`, `<stdarg.h>`, `<stdatomic.h>`, ...). They are built on compiler predefined macros (`__INT32_TYPE__`, `__FLT_MANT_DIG__`, `__builtin_va_list`, `__atomic_*`, ...). The library and its tests compile with `-nostdinc -ffreestanding`. | It gives one consistent header set across GCC and Clang. There is no leakage from host glibc headers. |
| D9 | **Toolchain floor: GCC ≥ 14 or Clang ≥ 19** (the first versions with usable `-std=c23`, `constexpr`, `typeof`, `[[attributes]]`, `nullptr`, `_BitInt`). On the Ubuntu 24.04 container use `apt install gcc-14` (GCC 13 rejects `-std=c23`). For ARM, use the `arm-none-eabi` 15.2 toolchain that `setup.mk` already sketches. | Required for the C23 target. |
| D10 | **The TLS model is static `local-exec`.** libc provides `__libc_tls_size`, `__libc_tls_align` and `__libc_tls_init(void *block)`, which copy `.tdata` and zero `.tbss` from linker symbols. The port allocates the per-thread block and sets the thread pointer (x86_64 `%fs`; ARM `__aeabi_read_tp`, provided by the port). | There is no dynamic loader on embedded targets. It is the cheapest correct `thread_local`. |
| D11 | **Heap:** libc owns `malloc` (a clean-room allocator, see the research step). It gets memory from `__rtos_heap_region()`/`__rtos_sbrk()` and is protected by a seam lock. An RTOS may replace `malloc` wholesale by linking its own definitions ahead of libc.a. | Deterministic behaviour matters on embedded systems, and replacing the allocator must stay possible. |
| D12 | **Size and speed flavours** are chosen per build without `#if`. Speed-oriented arch files live in `arch/<arch>/speed/...` and size-oriented ones in `arch/<arch>/size/...`. The build picks a flavour directory (`LIBC_FLAVOUR=size|speed`, default `size`). | It reconciles Phase 2 and Phase 3. Needs confirmation (see Q3). |
| D13 | **Feature trimming is link-time.** Expensive optional paths, such as floating point in `printf`/`scanf`, are reached through a symbol that the full variant defines and a small variant stubs out. The small variant lives in a separate archive (`libc-nofloat.a`) linked ahead of `libc.a`. It is never selected with `#if`. | This is the picolibc-style flavour idea, done at link time. It is a Phase 2 item. |
| D14 | **Tracking:** phase files plus a step ID per session. Each step lists its deliverables and its definition of done. Sessions update the checkboxes, the test baselines and the decision log, then commit. | The work spans many sessions. |
| D15 | **Headers are per target triple, not per multilib.** Every multilib of one GCC (e.g. all `arm-none-eabi` M-profile variants) shares one `include/` directory, so `arch/arm/include/` must be valid for all of them. Types such as `jmp_buf` are sized for the largest variant (with the VFP callee-saved registers: s16–s31 for single precision and d8–d15 for double precision are the same 64 bytes); `fenv_t` (FPSCR) is the same for all. Only `arch/<arch>/src/` (code) differs per multilib. | Phase 4 needs it: GCC multilib installs one sysroot `include/` and per-multilib `lib/<multidir>/`. Deciding this early stops Phases 1 to 3 from baking per-arch header differences in. |
| D16 | **The build can be driven from outside.** A caller (crosstool-NG) can build and install one multilib with `make ARCH_CROSS=<arch> CROSS_COMPILE=<prefix> MULTILIB_FLAGS='<flags>' MULTIDIR=<dir> PREFIX=<sysroot> install`, and install headers alone with `make install-headers`. | crosstool-NG needs headers before the final GCC, and then one libc build per multilib. |
| D17 | **Arch code comes from ordered layers.** Each `tools/makefiles/<arch>.mk` sets `ARCH_LAYERS`, an ordered list of `arch/<layer>/src` directories. For each source stem the first layer that has the file wins; otherwise the generic `src/` file is used. Layers are per ISA (`armv6-m`, `armv7e-m`, `armv8-m.main`) and per FPU (`fpv4-sp`, `fpv5-sp`, `fpv5-dp`). For example, `armv7e-m-dp` uses `armv7e-m-dp fpv5-dp fpv5-sp armv7e-m`. This extends D5. | It shares FPU-specific code between ISAs (the Cortex-M7 DP unit and FPv5 on v8-M) and ISA-specific code between FPU variants, without duplicating files and without `#if`. |
| D18 | **Support the Cortex-M double-precision FPU (FPv5-D16, e.g. Cortex-M7 with the DP unit)** as the arch `armv7e-m-dp` (`-march=armv7e-m+fp.dp -mfloat-abi=hard`, i.e. FPv5-D16; D24). On it, `double` operations are hardware; `float` and `double` both get FPv5 instructions (`VSQRT`, `VFMA`, `VRINT[ANPMZX]`, `VMAXNM`/`VMINNM`, `VCVT[ANPM]`) through the `fpv5-sp`/`fpv5-dp` layers. Note that FPv5-SP (Cortex-M33, and the M7 SP option) also has these instructions for `float`, but FPv4-SP (Cortex-M4F) does not. | User requirement. It needs no header changes (D15 holds: the register file and FPSCR are shared). |
| D19 | **Exception to the no-`#if` rule:** `<limits.h>` selects `CHAR_MIN`/`CHAR_MAX` with `#ifdef __CHAR_UNSIGNED__`. Plain `char` signedness is a compiler option with no predefined *value* macro, and the limits must be usable in `#if`. Further compiler-property selections in headers need their own entry. | There is no conditional-free way to express it. |
| D20 | **`<errno.h>` values follow the Linux numbering** (`EDOM` 33, `ERANGE` 34, `EILSEQ` 84, `EBADF` 9, ...). Seam functions return negated errno values, so `port/linux` passes kernel results through unchanged, and other ports map their own codes. | Simplest for the test port. The values are arbitrary otherwise. |
| D21 | **Build layout:** `src/`, `arch/` and `include/` are source pools with no `.mk` files. The `libc/` module compiles them (through `libc/subdir.mk` → `tools/makefiles/libc.mk`), and the `port/<port>/` modules select from their own `src/` and `arch/<layer>/src/` the same way. `BUILD_ROOT` and `INSTALL_ROOT` include `ARCH_CROSS` (`build/<arch>/<type>`, `images/<arch>/<type>`). Consumers (`example/`, `test/`) get the libc include and link flags from `tools/makefiles/libc-target.mk`, which each `<arch>.mk` includes. | It keeps one compile rule for layered selection (D5, D17), and lets several arches coexist in one tree. |
| D22 | **Build system usage follows the pi-pico conventions** (supersedes D5, D17 and D21). (1) Every source directory is a module with its own `<dir>.mk` (from `tools/templates/src-template.mk`) and compiles in place. Library code lives under `libc/<area>/` and `port/<port>/`, and private headers in `libc/internal/` (header-only, no `.mk`). (2) Arch variation lives in variant subdirectories named after the `ARCH_CROSS` toolchain file (`libc/tls/x86_64/`, `port/linux/armv8-m.main/`). The parent module selects them with `SUBDIRS := ${ARCH_CROSS}` (as `sys/sys.mk` does in pi-pico). Code shared between cores goes in a subdirectory named by a variable the toolchain file sets (e.g. `LIBC_FPU := fpv5-dp`, `SUBDIRS := ${ARCH_CROSS} ${LIBC_FPU}`). When a generic C function needs an arch override (Phase 2/3), the generic module's `subdir.mk` filters it out of `SRC`. (3) Archives are built by a parent module that lists its children in `TARGET_OBJ_LIBS` with `all: ${CURDIR}/libX.a` (`libc/libc.mk` → `libc.a`, `port/linux/linux.mk` → `librtos-linux.a`, like pi-pico `runtime.mk`). (4) Executables list the object directories they link in `TARGET_OBJ_LIBS` (e.g. `port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_CROSS}`). The toolchain file links `libc.a` from the build tree through `LDLIBS`, as pi-pico's toolchain file points at its build-tree picolibc. (5) A `BOARD_TYPE` file (`tools/makefiles/<board>.mk`) exports `ARCH_CROSS` and `RTOS_PORT`. The `ARCH_CROSS` file (`tools/makefiles/<arch>.mk`) holds the toolchain and all flags, including `LIBC_CFLAGS` for library modules. The top-level `Makefile` loops over `BOARDS` × `BUILDS` into `build/<board>/<build-type>`. (6) Builds always run from the project root. | Required by the user: the build system in `tools/` must be used the way pi-pico uses it. |
| D23 | **Tests run with `make check`** (amends D22 (6)): the top-level `Makefile` builds, then runs `test/unit/run-unit` on `${BUILD_ROOT}/test/unit` for every board and build type. libc-test joins the same target in P1.02. There is no `tools/test-runner`. | User direction. |
| D24 | **`ARCH_CROSS` toolchain files and variant subdirectories are named after the architecture, not a core** (amends D22): `x86_64`, `armv6-m`, `armv7e-m`, `armv7e-m-dp`, `armv8-m.main`, with test boards `linux-<arch>`. The ARM toolchain files select the target with `-march` plus extensions (e.g. `armv7e-m+fp`, `armv7e-m+fp.dp`, `armv8-m.main+fp+dsp`) and `-mfloat-abi`, never `-mcpu`. That way the library is not tuned for one core, and the names match the GCC multilib directories used in Phase 4. | User direction. |
| D25 | **Lowercase everywhere unless the standard requires uppercase.** Applies to our identifiers, constants (as `static constexpr`, not macros), include guards (`__<name>_h`), file names (`_Exit` is in `_exit.c`) and test output. The exceptions are standard-mandated names (`_Exit`, `FILE`, the limit macros, `E*` errno macros per C23 7.5p4) and compiler/toolchain names (`__INT32_TYPE__`, `.note.GNU-stack`). This refines D6, which says the file is named after its function. | User direction. |

---

## 3. Repository layout (target)

```
CLAUDE.md
Makefile                  top-level: BOARDS x BUILDS loop, module ordering (D22)
tools/                    build system (extend, don't restructure)
  makefiles/<board>.mk    BOARD_TYPE: exports ARCH_CROSS and RTOS_PORT (linux-x86_64, ...)
  makefiles/<arch>.mk     ARCH_CROSS toolchain + flags (x86_64, armv6-m, armv7e-m, armv7e-m-dp, armv8-m.main)
include/                  public headers (no .mk)
  rtos/rtos-port.h        the RTOS seam contract (installed for port authors)
libc/libc.mk              tree node; archives its children into libc.a (+ libm.a)
libc/<area>/              one module per area, one function per file
libc/<area>/<arch>/       ARCH_CROSS variant subdirectory, selected with SUBDIRS (setjmp, fenv, tls, ...)
libc/internal/            private headers (never installed, no .mk)
port/linux/               RTOS seam over Linux syscalls (test port) + port/linux/<arch>/ variants
port/none/                single-thread, bare-metal default port (no-op locks, one static TLS block)
test/libc-test/           build glue, shim, runner, manifests, expected results per arch
test/unit/                our own tests for gaps in libc-test (C23 additions, threads.h, seam)
test/bench/               Phase 3 benchmarks
toolchain/crosstool-ng/   Phase 4: crosstool-NG libc backend, sample configs, patches
third-party/libc-test     git submodule (test suite only, never linked into libc)
docs/standard/            C23 standard text (N3220 clause 7, Annex B)
docs/plan/                this plan and the phase task lists
docs/research/            per-algorithm research notes (D2)
docs/size/, docs/perf/    baselines and reports (Phase 2, Phase 3)
example/                  existing example
```

Each source directory has a `<dir>.mk` made from `tools/templates/` (use
`tools/scripts/mksrc <path>`). See D22 for variants, archives and linking.

---

## 4. The RTOS seam (contract sketch)

The exact signatures are finalised in step P1.01 (bootstrap), P1.04 (heap, locks), P1.09 (I/O), P1.11 (time) and P1.12 (threads).
All the functions are `extern`, have the prefix `__rtos_`, and are implemented by
exactly one port.

| Group | Functions (draft) | Used by |
|---|---|---|
| Process | `__rtos_exit(int)` `[[noreturn]]`, `__rtos_abort(void)` `[[noreturn]]` | `exit`, `_Exit`, `quick_exit`, `abort` |
| TLS | the port calls `__libc_tls_init()`; the port provides the thread pointer (`__aeabi_read_tp` on ARM, `%fs` on x86_64) | `thread_local` |
| Locks | `__rtos_lock_init/acquire/release/destroy(__rtos_lock_t *)` (recursive) and a static initializer macro in `rtos-port-types.h` | malloc, stdio, atexit, `getenv`, time zone state |
| Threads | `__rtos_thread_create/join/detach/exit/yield/sleep/self/equal`, `__rtos_mutex_*` (plain/timed/recursive), `__rtos_cond_*` | `<threads.h>` |
| Time | `__rtos_clock_gettime(int base, struct timespec *)`, `__rtos_clock_getres(...)`, `__rtos_cpu_time()` | `time`, `timespec_get(res)`, `clock`, `thrd_sleep` |
| I/O | `__rtos_open/close/read/write/seek/remove/rename` (handle-based; a port with no file system implements only the std streams) | `<stdio.h>` |
| Heap | `__rtos_heap_region(void **base, size_t *size)` or `__rtos_sbrk(ptrdiff_t)` (chosen in P1.04) | `malloc` |
| Env | `__rtos_getenv(const char *)`, `__rtos_system(const char *)` | `getenv`, `system` |

`port/none` must link and run single-threaded with stdout going to a
user-replaceable byte sink. That way a bare-metal image works with no RTOS at all.

---

## 5. Test strategy

- **libc-test** (`third-party/libc-test`, pinned at commit
  `7b95dfa5f5d5ca4d949221e0228ccc290bacc14e`). It is the main gate.
  - `test/libc-test/manifest.txt` lists every libc-test source with a status of
    `run`, `skip:<reason>` (POSIX-only, wide characters, fork, ...) or
    `pending:<step>` (not implemented yet).
  - `test/libc-test/shim/` replaces libc-test's POSIX-dependent `src/common`
    pieces (for example `t_printf` built on `write`, and `runtest` built on
    fork/exec) with ISO/port equivalents. This is test-only code.
  - The runner `test/libc-test/run` runs each test binary with a timeout (under
    `qemu-arm -cpu …` for ARM) and writes `build/<board>/<type>/test/libc-test/results.txt`.
  - `test/libc-test/expected/<board>.txt` holds the committed baseline. The libc-test runner
    fails on any PASS→FAIL regression and reports FAIL→PASS improvements. A
    step updates the baseline only to record improvements.
  - `src/api` tests (compile-only declaration checks) are curated to their ISO subset.
- **test/unit**: our own tests for things libc-test misses: C23 additions
  (`<stdbit.h>`, `<stdckdint.h>`, `memset_explicit`, `strfromd`, `timegm`,
  `free_sized`, ...), `<threads.h>`, per-thread `errno` and state, the seam
  contract, and `port/none`.
- **Header audit** (P1.17): a generated check that every identifier and macro in
  Annex B is declared with the right type, and that every `__STDC_VERSION_*_H__` is
  `202311L`.
- **qemu-user caveat:** `qemu-arm` runs M-profile *instruction subsets* on an
  A-profile CPU model. That is fine for libc code (no system registers), but
  ports must not use M-profile-only instructions in the Linux test port. An
  optional semihosted `qemu-system-arm` (mps2-an385/an505) gate can be added later.

---

## 6. Session workflow

See CLAUDE.md. In short: pick the first `[ ]` step in the active phase file,
mark it `[~]`, do the research notes, implement, run `make check` for every
enabled board, update the manifest and the baseline, tick the boxes, append any
decisions here, and commit.

---

## 7. Open questions

| ID | Question | Default until answered |
|---|---|---|
| Q1 | Accept D7 (`-mlong-double-64` on x86_64)? | Yes |
| Q2 | ~~libc-test cannot be fetched (`repo.or.cz` denied).~~ **Resolved 2026-10-18:** the network policy now allows `repo.or.cz`, and the submodule is checked out at the pinned commit. | — |
| Q3 | Should size and speed be separate library flavours (D12), or one library tuned for size with speed only where it costs nothing? | Separate flavours |
| Q4 | What does the rtoskit RTOS port need (is a repo available, which primitives, TLS handling on Cortex-M)? It would be a `port/rtoskit` step after P1.12. | Not planned yet |
| Q5 | Phase 3 measurement on ARM: qemu instruction counts (TCG plugin) only, or also real hardware cycle counts (RP2040 / RP2350 with the DWT cycle counter)? | qemu instruction counts, plus hardware if a board is attached |
| Q6 | Time zone support for `localtime`/`mktime`: UTC only, or the POSIX `TZ` string from `getenv("TZ")`? | POSIX `TZ` string, no tzdata files |
| Q7 | ~~Is extending `tools/makefiles` with new files acceptable?~~ **Resolved by D22:** follow the pi-pico conventions, and add only board and `ARCH_CROSS` toolchain files there. | — |
| Q8 | Must the Phase 4 toolchain include C++ (`libstdc++`)? `libstdc++` expects `<wchar.h>`/`<wctype.h>`, which are out of scope, so it would need `--disable-wchar_t`, a C-only toolchain, or a scope change. | C-only toolchain (`--enable-languages=c`); C++ is a later decision |
| Q9 | "cross-ng" is taken to mean **crosstool-NG**. Where does the integration live: patches in this repo (`toolchain/crosstool-ng/`), a crosstool-NG fork, or the existing `red-rocket-computing/cross-dev` toolchain build that `tools/makefiles/setup.mk` downloads from? Which multilib set: GCC's `rmprofile` list, or a custom list limited to the ARM arches in the target matrix? | Patches in this repo; custom multilib list (v6-m, v7e-m+fp, v7e-m+fp.dp, v8-m.main+fp, plus their soft-float variants) |
| Q10 | Beyond the Cortex-M7 DP unit, should the Armv8.1-M cores with an optional DP FPU and MVE (Cortex-M55/M85, `armv8.1-m.main+fp.dp+mve.fp`) be a target too? With D17 that would add an `armv8.1-m.main` layer and reuse `fpv5-dp`. | No; add later if needed |
