# CLAUDE.md — rtoskit-libc

A clean-room, ISO C23 (N3220) compliant C standard library for deeply embedded
systems, bound to an RTOS through a link-time seam. Licence: MPL-2.0.

**Read first, every session:** `docs/plan/PLAN.md` (goals, architecture, the
decision log D*, open questions Q*), then the active phase file
(`docs/plan/phase-{1,2,3,4}.md`).

## Session workflow

1. Find the active phase: the first phase file with an unticked step. Take the
   first `[ ]` step (or resume a `[~]` one) and mark it `[~]`. One step per
   session. If a step is too big, split it in the phase file before starting.
2. Mirror the step's checklist into the session task list (TaskCreate) and keep it current.
3. **Research before code (mandatory).** For every algorithm, review existing
   implementations on the web (musl, picolibc, newlib, glibc, Bionic,
   FreeBSD/OpenBSD, ARM optimized-routines, CORE-MATH, papers). Write
   `docs/research/<topic>.md` with the URLs, the trade-offs, the chosen
   approach, the edge cases and the test vectors.
4. **Clean-room (mandatory).** Never copy, paste or mechanically translate code
   from other implementations, whatever their licence. Write from the
   standard text (`docs/standard/library-7.pdf`, `docs/standard/annex-b.pdf`; extract text with
   `pdftotext -layout`) and your own research notes.
5. Implement, then build and validate on **every enabled arch** (see the phase
   file): `make` and `make check`.
6. Update `test/libc-test/manifest.txt` and `test/libc-test/expected/<arch>.txt`
   (improvements only; a PASS→FAIL is never accepted into the baseline).
7. Tick the checkboxes, append any new decisions to the PLAN.md decision log
   (never rewrite old entries; supersede them), and record open questions as Q*.
8. Commit with a message prefixed by the step ID, e.g. `P1.03: string.h word-at-a-time functions`.

Never mark a step `[x]` with failing builds, libc-test regressions or missing
research notes. If blocked, leave it `[~]`, write the blocker in the step, and
tell the user.

## Build (tools/ build system — must be used)

```sh
make                                   # debug + release for ARCH_CROSS (default x86_64 once P1.01 lands)
make BUILD_TYPE=debug                  # single build type
make ARCH_CROSS=armv7e-m               # cross arch: x86_64 | armv6-m | armv7e-m | armv7e-m-dp | armv8-m.main
make libc                              # one module (dep-free, assumes prior full build)
make check                             # build + run libc-test + diff vs expected (P1.02+)
make size-report                       # Phase 2+
make bench                             # Phase 3+
make info | help | clean | distclean
V=1 make ...                           # verbose
```

- The output goes to `build/<build_type>/...` and the installs to `images/<build_type>/`.
- Every source directory needs a `<dirname>.mk`. Create one with
  `tools/scripts/mksrc <path>`, or copy from `tools/templates/`. A directory with
  only `.mk` files is a tree node (`tree-template.mk`); a directory with sources
  is a project (`src-template.mk`, optionally with a `subdir.mk`).
- Modules are discovered automatically (`*/<name>.mk`). The cross-module order
  is set in the top-level `Makefile` (`target:` dependencies).
- Extend the build system by **adding** files under `tools/makefiles/`
  (toolchains `<arch>.mk`, the shared `libc.mk`). Do not restructure the
  existing tools makefiles.
- Toolchain floor: GCC ≥ 14 / Clang ≥ 19 (D9). On Ubuntu 24.04: `apt-get install -y gcc-14 qemu-user gcc-arm-none-eabi`.

## Code rules

- **C23** (`-std=c23`): use `nullptr`, `bool`, `constexpr`, `static_assert`,
  `[[noreturn]]`/`[[nodiscard]]`/`[[maybe_unused]]`, `typeof`, and `thread_local`.
  The whole library builds `-nostdinc -ffreestanding -fno-builtin` against its
  own headers only (D8).
- **Per-thread state is `thread_local`:** `errno`, `strtok`, `rand`, the
  `asctime`/`ctime`/`gmtime`/`localtime` results, `tmpnam` and so on. Each one
  is a separate variable in the file of its only user (D6). Do not add global
  mutable state without a seam lock or a `thread_local` declaration.
- **One external function per `.c` file**, with the file named after the
  function (D6). Internal helpers shared between files go in
  `src/internal/*.h` with `__libc_` names, declared `static inline` or hidden.
- **Reserved names only:** internal symbols use `__libc_*`, the seam uses
  `__rtos_*`, and arch/port header pieces are named `__arch_*.h` / `rtos-port-types.h`.
  No non-standard identifier may leak from public headers unless it is reserved
  (`__x`, `_X`).
- **Headers must not differ per multilib** (D15). Anything ABI-visible in
  `arch/<family>/include` must be identical for every arch of that triple.
  Size `jmp_buf` and similar types for the largest variant.
- **The RTOS seam** (D4): the library reaches the OS only through the `__rtos_*`
  functions declared in `include/rtos/rtos-port.h`. No syscalls and no port
  knowledge belong in `src/`. A port implements the seam in `port/<port>/`.

### Conditional compilation (minimize — user requirement)

Allowed `#if`/`#ifdef` uses, and nothing else:

1. Include guards.
2. `__cplusplus` `extern "C"` wrappers in public headers.
3. Macros the standard requires to be conditional (`NDEBUG` in `<assert.h>`,
   `__STDC_WANT_*`, and feature macros the standard says are defined only
   when supported).

Instead of `#if`:

- Arch/port variation means a different file selected by the build
  (the first match by stem in the arch layer list, then `src/...`; arch
  headers come through the include path) (D5, D17).
- Feature or flavour variation means a separate archive or a link-time symbol
  replacement (D12, D13).
- Type-size variation means plain `if (sizeof(long) == 8)`, `constexpr` and
  `static_assert`, which the compiler folds.

Any exception needs a PLAN.md decision entry.

### Style

- Format with `tools/clang-format` (tabs, width 4, function braces on their own
  line, `PointerAlignment: Right`).
- New files start with the MPL header from
  `tools/templates/mpl-c-code-header.txt`. Fill in `<filename>` and `<date>`;
  the author is Stephen Street.
- Comment the *why*: standard clause references (`/* C23 7.24.2.1p2 */`) and
  edge cases. No commented-out code.
- Builds are `-Wall -Werror` clean on every arch with GCC (and with Clang, if
  enabled).

## Testing

- **libc-test** (`third-party/libc-test`, a submodule) is the conformance gate
  at every step of every phase (PLAN §5). Run it natively on x86_64 through
  `port/linux`, and under `qemu-arm` for the ARM arches.
- `test/libc-test/manifest.txt` gives every libc-test file a status: `run`,
  `skip:<reason>` (out of scope: POSIX, wide chars, fork) or `pending:<step>`.
- `test/libc-test/shim/` is test-only replacement code for libc-test's POSIX
  helpers. It is never part of `libc.a`.
- `test/unit/` holds our own tests for everything libc-test misses (the C23
  additions, `<threads.h>`, thread-local state, the seam, `port/none`).
- Never skip, disable or weaken a test to get green. Fix the code, or record a
  justified `skip:` that refers to the scope in PLAN.md §1.

## Environment notes

- Fetch libc-test with `git submodule update --init` (`repo.or.cz` must be
  reachable). If the network blocks it, report the blocker. Never substitute
  made-up results.
- `docs/standard/*.pdf` is the standard text used for clean-room work.

## Layout

```
include/           public headers (+ include/rtos/rtos-port.h seam contract)
src/<area>/        generic code, one function per file; src/internal/ private headers
arch/<family>/include/  __arch_*.h, one header set per target triple (arm, x86_64), shared by all multilibs (D15)
arch/<layer>/src/  code overrides; each ARCH_CROSS searches an ordered list of layers, e.g. armv7e-m-dp → fpv5-dp → armv7e-m → src (D17); size/ and speed/ flavours (Phase 2/3)
port/<port>/       linux (test port), none (bare metal), rtoskit (future)
libc/              archives libc.a / libm.a
test/              libc-test glue, unit, size, bench
toolchain/         Phase 4: crosstool-NG libc backend, multilib configs
docs/standard/     C23 standard text (N3220 clause 7, Annex B)
docs/plan/         PLAN.md + phase task lists
docs/research/     per-algorithm research notes
tools/             build system
third-party/       libc-test submodule
```
