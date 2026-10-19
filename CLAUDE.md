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
   board): `make` and `make check`.
6. Update `test/libc-test/manifest.txt` and `test/libc-test/expected/<board>.txt`
   (improvements only; a PASS→FAIL is never accepted into the baseline).
7. Tick the checkboxes, append any new decisions to the PLAN.md decision log
   (never rewrite old entries; supersede them), and record open questions as Q*.
8. Commit with a message prefixed by the step ID, e.g. `P1.03: string.h word-at-a-time functions`.

Never mark a step `[x]` with failing builds, libc-test regressions or missing
research notes. If blocked, leave it `[~]`, write the blocker in the step, and
tell the user.

## Build (tools/ build system — must be used, pi-pico conventions, D22)

Always run `make` from the project root; never `make -C <subdir>` or `make -f <subdir>/x.mk`.

```sh
make                                   # every board in BOARDS x every build type in BUILDS
make BUILD_TYPE=debug                  # debug only (all boards)
make BOARD_TYPE=linux-x86_64           # one board
# One module (dep-free, assumes a prior full build); needs a concrete board and build type
make BOARD_TYPE=linux-x86_64 BUILD_TYPE=debug libc
make BOARD_TYPE=linux-x86_64 BUILD_TYPE=debug libc/string
make BOARD_TYPE=linux-x86_64 BUILD_TYPE=debug test/unit/smoke
make BOARD_TYPE=linux-x86_64 BUILD_TYPE=debug info
make help | clean | distclean
V=1 make ...                           # verbose
make check                             # build, then run the unit tests (+ libc-test from P1.02) for every board and build type
make BOARD_TYPE=linux-x86_64 BUILD_TYPE=debug check
```

- **Boards and toolchains:** `BOARD_TYPE` files (`tools/makefiles/linux-x86_64.mk`,
  `linux-armv6-m.mk`, `linux-armv6s-m.mk`, `linux-armv7e-m.mk`, `linux-armv7e-m-dp.mk`,
  `linux-armv8-m.main.mk`) export `ARCH_CROSS`, `RTOS_PORT`, and for the Arm boards
  `TARGET_RUNNER` (`qemu-arm -cpu max`, D36) and `BOARD_LDFLAGS`. The `ARCH_CROSS`
  file (`tools/makefiles/x86_64.mk`, `armv6-m.mk`, `armv6s-m.mk`, `armv7e-m.mk`, `armv7e-m-dp.mk`,
  `armv8-m.main.mk`; architecture names, `-march` not `-mcpu`, D24) sets the
  toolchain and all flags, including `LIBC_CFLAGS` for library code, and exports
  `ARCH_FAMILY` (`x86_64`, `arm`), on Arm `LIBC_FPU` (D34), `LIBC_FENV` (the `<fenv.h>`
  variant, D44) and `LIBC_MATH_LAYERS` (libm FPU layers, D46). The Arm files
  share `arm-none-eabi.mk`. It also links `libc.a` from the build tree through
  `LDLIBS`.
- **Arm toolchain:** arm-none-eabi GCC 16.2 lives in `local/` (gitignored). The
  first build downloads it (`make setup`, `tools/makefiles/setup.mk`).
- **Allocator:** `LIBC_MALLOC` picks the allocator built into `libc.a`:
  `nano` (default, set in the board file) or `tlsf`
  (`make LIBC_MALLOC=tlsf ...`; run `make clean` after changing it). Both
  variants under `libc/malloc/` are always built and tested (D32).
- **Output:** `build/<board>/<build-type>/<module path>/`; installs go to
  `images/<board>/<build-type>/`.
- **Modules:** every source directory is a module with its own `<dirname>.mk`
  (`tools/scripts/mksrc <path>` or `tools/templates/src-template.mk`) and
  compiles in place. A directory holding only sub-modules is a tree node.
  Library modules add `CPPFLAGS += -I${PROJECT_ROOT}/libc` and
  `CFLAGS += ${LIBC_CFLAGS}` after including `project.mk`.
- **Arch variants:** put code that depends on the core (ISA, FPU) in a
  subdirectory named after `ARCH_CROSS` (`libc/setjmp/armv7e-m/`), and code
  shared by every core of a target triple in one named after `ARCH_FAMILY`
  (`libc/tls/arm/`, `port/linux/arm/`, D34). The parent module selects it with
  `SUBDIRS := ${ARCH_CROSS}` (or `${ARCH_FAMILY}`) in its target branch. A test
  that only applies to some targets selects its sources in a `subdir.mk`
  (`test/unit/fpv5`).
- **Archives:** a parent module lists its children in `TARGET_OBJ_LIBS` and adds
  `all: ${CURDIR}/libX.a` (`libc/libc.mk`, `port/linux/linux.mk`). Register
  every new libc module or variant in `libc/libc.mk`.
- **Executables:** list what they link in `TARGET_OBJ_LIBS`, e.g.
  `port/${RTOS_PORT} port/${RTOS_PORT}/${ARCH_FAMILY}`, set
  `EXTRA_ELF_DEPS := ${BUILD_ROOT}/libc/libc.a` so the executable relinks when
  the library changes, then add `all: ${CURDIR}/<name>.elf`.
- **Module order:** set in the top-level `Makefile` (`example test: libc port`).
- **Extending the build system:** add board and toolchain files under
  `tools/makefiles/`. Do not restructure the existing tools makefiles.
- **Toolchain floor:** GCC ≥ 14 / Clang ≥ 19 (D9). On Ubuntu 24.04:
  `apt-get install -y gcc-14 qemu-user`; arm-none-eabi GCC 16.2 from `make setup`.

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
  `libc/internal/*.h` with `__libc_` names, declared `static inline` or hidden.
- **Naming (D26, Linux kernel style as in pi-pico):** identifiers and file names
  are `lower_case_with_underscores`; macros and enum constants are `ALL_CAPS`;
  include guards are `#ifndef _NAME_H` / `#define _NAME_H` /
  `#endif /* _NAME_H */`, with a leading underscore so public guards stay in the
  reserved namespace. Names the standard fixes keep its spelling (`_Exit`,
  `FILE`, `INT32_MAX`). A file defining a standard function with uppercase
  letters gets a lowercase file name (`_Exit` lives in `libc/stdlib/_exit.c`).
- **Reserved names only:** internal symbols use `__libc_*`, the seam uses
  `__rtos_*`, and arch/port header pieces are named `__arch_*.h` / `rtos-port-types.h`.
  No non-standard identifier may leak from public headers unless it is reserved
  (`__x`, `_X`).
- **Headers must not differ per multilib** (D15). Anything ABI-visible in a
  public header must be identical for every core of a target triple. Size
  `jmp_buf` and similar types for the largest variant.
- **The RTOS seam** (D4): the library reaches the OS only through the `__rtos_*`
  functions declared in `include/rtos/rtos-port.h`. No syscalls and no port
  knowledge belong in `libc/`. A port implements the seam in `port/<port>/`.

### Conditional compilation (minimize — user requirement)

Allowed `#if`/`#ifdef` uses, and nothing else:

1. Include guards.
   (Plus the D19 exception: `CHAR_MIN`/`CHAR_MAX` in `<limits.h>`.)
2. `__cplusplus` `extern "C"` wrappers in public headers.
3. Macros the standard requires to be conditional (`NDEBUG` in `<assert.h>`,
   `__STDC_WANT_*`, and feature macros the standard says are defined only
   when supported).

Instead of `#if`:

- Arch/port variation means a variant subdirectory selected by the build
  (`SUBDIRS := ${ARCH_CROSS}`, D22).
- Feature or flavour variation means a separate archive or a link-time symbol
  replacement (D12, D13).
- Type-size variation means plain `if (sizeof(long) == 8)`, `constexpr` and
  `static_assert`, which the compiler folds.

Any exception needs a PLAN.md decision entry.

### Style

- **Linux kernel coding style** (`Documentation/process/coding-style.rst`), as
  pi-pico applies it:
  - Long lines are allowed (`ColumnLimit: 0`).
  - K&R braces, with function braces on their own line.
  - `/* */` comments only; never `//`.
  - No alignment padding (never align `=`, fields or trailing comments).
  - `while (true)` for endless loops; never `for (;;)`.
  - Name status-return variables `status`.
  - No new typedefs except where the standard requires them.
  - A blank line after declarations.
  - No `extern` declarations in `.c` files; put them in a header.
- **Formatting:** run `clang-format -i` (`.clang-format` → `tools/clang-format`)
  on every C/H file you edit. It is authoritative for mechanical layout (tabs,
  indented case labels, `while (true);`).
- **Optional style check:** the kernel's `scripts/checkpatch.pl --no-tree --file
  --max-line-length=255 --ignore SPDX_LICENSE_TAG`, kept outside the repo.
  Findings that standard-mandated code causes are expected (the `<stdint.h>`
  typedefs, `<iso646.h>`).
- New files start with the MPL header from
  `tools/templates/mpl-c-code-header.txt`. Fill in `<filename>` and `<date>`;
  the author is Stephen Street.
- Comment the *why*: standard clause references (`/* C23 7.24.2.1p2 */`) and
  edge cases. No commented-out code.
- Builds are `-Wall -Werror` clean on every arch with GCC (and with Clang, if
  enabled).

## Testing

- **libc-test** (`third-party/libc-test`, a submodule) is the conformance gate
  at every step of every phase (PLAN §5). Run it natively on `linux-x86_64`
  through `port/linux`, and under `qemu-arm -cpu max` (`TARGET_RUNNER`) for the
  five ARM boards (D36, D37).
- `make check` builds, then runs every unit test through `test/unit/run-unit`. A
  test passes when it exits with 0, unless an executable `<name>.run` next to
  its source decides instead (see `test/unit/smoke/smoke.run`). `TARGET_RUNNER`
  (e.g. qemu-arm) prefixes each execution.
- `test/libc-test/manifest.txt` gives every libc-test file a status: `run`,
  `skip:<reason>` (out of scope: POSIX, wide chars, fork) or `pending:<step>`.
  Flipping a test to `run` is all it takes to build it: the suite modules
  `test/libc-test/{api,functional,math,regression}` build one ELF per `run`
  test through `tools/makefiles/libc-test.mk` (D27).
- `make check` compares the libc-test results with
  `test/libc-test/expected/<board>.txt` and fails on PASS→FAIL and on failures
  missing from the baseline. Record improvements with
  `make check LIBC_TEST_UPDATE=1` (it refuses while anything regressed).
- `test/libc-test/shim/` is test-only replacement code for libc-test's POSIX
  helpers. It is never part of `libc.a`.
- `test/unit/` holds our own tests for everything libc-test misses (the C23
  additions, `<threads.h>`, thread-local state, the seam, `port/none`).
  Unit-test modules add `CFLAGS += -fno-builtin` after `project.mk` so the
  library, not the compiler's built-in folding, is what runs.
- **Size (phase 2, D51):** `test/size/` holds the reference programs (built, not
  run). After a release build, `tools/scripts/size-report diff` compares the
  members, images and stack depths with `docs/size/baseline-<arch>.json` and
  exits 1 on any growth; `size-report report` rewrites the baselines once a
  step's reductions are recorded in `docs/size/<arch>.md`.
- Never skip, disable or weaken a test to get green. Fix the code, or record a
  justified `skip:` that refers to the scope in PLAN.md §1.

## Environment notes

- Fetch libc-test with `git submodule update --init` (`repo.or.cz` must be
  reachable). If the network blocks it, report the blocker. Never substitute
  made-up results.
- `docs/standard/*.pdf` is the standard text used for clean-room work.

## Layout

```
include/           public headers (no .mk; + include/rtos/rtos-port.h seam contract)
libc/              libc.a: libc/<area>/ modules, one function per file
libc/<area>/<arch>/ ARCH_CROSS variant subdirectory (SUBDIRS := ${ARCH_CROSS})
libc/<area>/<family>/ ARCH_FAMILY variant shared by a triple's cores (libc/tls/arm/)
libc/internal/     private headers (no .mk)
port/<port>/       linux (test port), none (bare metal), rtoskit (future); port/<port>/<family>/ variants
test/              unit tests (test/unit/<name>/), libc-test glue, size, bench
example/           examples
toolchain/         Phase 4: crosstool-NG libc backend, multilib configs
tools/             build system (makefiles, templates, scripts)
docs/standard/     C23 standard text (N3220 clause 7, Annex B)
docs/plan/         PLAN.md + phase task lists
docs/research/     per-algorithm research notes
third-party/       libc-test submodule
```
