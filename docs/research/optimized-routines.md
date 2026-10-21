# Research: Arm Optimized Routines as a comparison point

Step: P4.01. Decision: PLAN D92 (the plan for this phase, under D91).

Arm Optimized Routines (AOR) is fetched, built and linked **only** into
measurement programs (D91). It is never copied into `libc/`, never translated,
and never used as a model for our code. This note records what AOR provides,
not how it does it. Its algorithms matter here only as far as they explain a
measured difference (P4.06).

## Sources

- Repository: <https://github.com/ARM-software/optimized-routines>, pinned at
  `f67e7daa5457646fad9f75580263dba654c988aa` (2026-10-16, "math: share
  wider-reference ULP setup").
- Licence: MIT OR Apache-2.0 WITH LLVM-exception (`LICENSE`).
- The top-level `README.md`, `config.mk.dist`, and the `string/`, `math/` and
  `fp/` `README.md` and `Dir.mk` files.
- The ulp claims below come from each routine's header comment and its
  `TEST_ULP` line.
- How to fetch it: GitHub's archive endpoint is not reachable from this
  environment, which returns "GitHub access to this repository is not enabled"
  for `/archive/<rev>.tar.gz`. A shallow `git fetch --depth 1 <url> <rev>` does
  work, and the commit ID is the content check.

## The three subprojects

### `string/`: string.h routines

- **Symbol names:** every routine has an implementation-specific name, such as
  `__memcpy_arm`, `__strcmp_armv6m` or `__strlen_armv6t2`
  (`string/include/stringlib.h`). AOR does not export `memcpy`. The
  comparison renames the symbols when it builds the archive
  (`objcopy --redefine-sym`).
- **x86_64:** `string/x86_64/` holds only `check-arch.S`. There is nothing to
  compare on x86_64.
- **Per core:** each file was trial-compiled with our GCC 16.2 and each board's
  `CROSS_FLAGS`.

| File | Symbol | armv6-m / armv6s-m | armv7e-m, -dp, armv8-m.main | Notes |
|---|---|---|---|---|
| `memchr.S` | `__memchr_arm` | assembles, but does not run (`.arch armv7-a` overrides `-march` below Armv8-M) | **yes** | Thumb-2 |
| `memset.S` | `__memset_arm` | assembles, but does not run (`.arch armv7-a`) | **yes** | Thumb-2 |
| `memcpy.S` | `__memcpy_arm` | no | no | A32 (`.arm`): "selected processor does not support ARM opcodes" on every M-profile core |
| `strcmp.S` | `__strcmp_arm` | empty | empty | Guarded by `__ARM_ARCH_ISA_ARM`, which is false on M-profile |
| `strcmp-armv6m.S` | `__strcmp_armv6m` | **yes** | empty | Guarded by `__ARM_ARCH_6M__` |
| `strlen-armv6t2.S` | `__strlen_armv6t2` | empty | **yes** | Guarded by `__ARM_ARCH_ISA_THUMB == 2` |
| `strcpy.c` | `__strcpy_arm` | empty | empty | Guarded by `__thumb2__ && !__thumb__`: A32 only |

What can be compared on M-profile:

- **armv6-m and armv6s-m:** `strcmp`.
- **armv7e-m, armv7e-m-dp and armv8-m.main:** `memchr`, `memset` and `strlen`.

There is no `memcpy`, `memmove`, `strchr` or `strcpy` for M-profile. Those
remain ours in every comparison build.

### `math/`: scalar libm

- **Symbol names:** the standard ones (`expf`, `exp`, ...), plus glibc aliases
  (`__expf_finite`), which `USE_GLIBC_ABI=0` turns off.
- **Portable C** for every target. The `aarch64/` vector code is out of scope.
- **Build:** `-std=c99 -O3` with `-frounding-math -fexcess-precision=standard
  -ffp-contract=fast -fno-math-errno` (`config.mk.dist`).
- **Our headers need C23.** Against our headers (`-nostdinc -isystem include`),
  all 31 sources build only with `-std=c23`. With `-std=c99`, `tgamma128.c`
  stops at our `<stddef.h>`. The comparison builds with `-std=c23`; the
  semantics do not change.
- **`errno`:** `WANT_ERRNO=0` (no `errno`) matches our `math_errhandling ==
  MATH_ERREXCEPT` (D45). Both report errors only through the floating-point
  exceptions.
- **Doubles on single-precision cores:** the float functions compute in
  `double`. On armv6-m (no FPU) and on armv7e-m and armv8-m.main
  (single-precision FPU) every double operation is a soft-float call.
  Only armv7e-m-dp and x86_64 run them in hardware. This is the main thing the
  comparison will show.

| Function | AOR stated maximum error (round to nearest) | Ours (D45, D47, D57–D60, D86–D88) |
|---|---|---|
| `expf`, `exp2f` | 0.51 ulp | correctly rounded ¹ |
| `logf` | 0.82 ulp | correctly rounded ¹ |
| `log2f` | 0.76 ulp | correctly rounded ¹ |
| `log10f` | 0.80 ulp | correctly rounded ¹ |
| `powf` | 0.90 ulp | correctly rounded ¹ |
| `sinf`, `cosf`, `sincosf` | 0.56 ulp | correctly rounded ¹ |
| `erff` | 1.09 ulp (comment), 1.1 (test limit) | correctly rounded ¹ |
| `exp`, `exp2` | 0.51 ulp | at most 1 ulp |
| `exp10` | 0.52 ulp | at most 1 ulp |
| `log` | 0.52 ulp | at most 1 ulp |
| `log2` | 0.55 ulp | at most 1 ulp |
| `pow` | 0.54 ulp | at most 1 ulp |
| `erf` | 1.01 ulp | at most 1 ulp |

¹ On armv7e-m-dp and x86_64, our float functions compute in double and
round correctly to nearest (D47). On the cores with a soft or
single-precision FPU (armv6-m, armv6s-m, armv7e-m, armv8-m.main), the
float-float layer `sp-nearest` is below 1 ulp, and at most a few results in
10^5 are not correctly rounded (D57). `test/unit/float_accuracy` enforces at
most 1 in 1000.

- **Not checked yet:** AOR's figures are its own claims, measured by its `ulp`
  tool against MPFR. P4.05 measures both libraries with one reference.
- **Other rounding modes:** AOR states 1 ulp (`TEST_ULP_NONNEAREST`). Ours keep
  their bounds in every mode on the cores where `<fenv.h>` changes the rounding
  mode.
- **Not covered by AOR (scalar, non-AArch64):** `tanf`, `tan`, `sin` and `cos`
  in double, `asin`/`acos`/`atan`, the hyperbolic functions, `cbrt`, `hypot`,
  `lgamma`/`tgamma` (except `tgamma128` for `long double`, which is 128-bit
  only and so not used on Arm or x86_64 here), `sqrt`, and the exact
  functions. These remain ours in every comparison build.

### `fp/`: soft-float arithmetic

- **Symbol names:** `arm_fp_<op>` (`arm_fp_fmul`, `arm_fp_dcmp_bool_eq`, ...).
  AOR deliberately avoids the run-time ABI names so that they can be linked
  next to a toolchain's own versions (`fp/README.md`). The comparison renames
  them to the RTABI names (`__aeabi_fmul`, `__aeabi_dcmpeq`, ...).
- **Semantics:** exact IEEE 754 results with denormals and signed zeros. There
  are no exception flags and no dynamic rounding mode. Arm's NaN conventions
  apply: the first operand's NaN wins, and an sNaN is quietened by setting the
  top mantissa bit. Out-of-range conversions saturate, and a NaN converts to 0.
  - libgcc's soft-float has the same contract.
  - Ours keeps its own contract: the speed flavour's
    `libc/math/nofp/__aeabi_{fmul,fdiv,fsub,dsub}.c` raise flags into the soft
    `<fenv.h>` state and honour its rounding mode.
  - So the comparison measures AOR against routines that do more work. The
    results tables must say so.
- **Per core:**
  - `fp/armv6-m/` (Thumb-1, also for Armv8-M Baseline): `fadd`/`fsub`/`frsub`,
    `fmul`, and the single and double comparisons (bool, flags, reversed
    flags, the GNU `__cmpsf2` forms). All 24 sources assemble for every core.
    It has no division, no double arithmetic and no conversions, so those
    remain libgcc's or ours on Armv6-M.
  - `fp/at32/` (A32 or Thumb-2): the complete set in float and double:
    add/sub/rsub, mul, div, comparisons, `f2d`/`d2f`, and every conversion to
    and from 32- and 64-bit integers. It assembles for armv7e-m, -dp and
    armv8-m.main with `-Wa,-mimplicit-it=always`, as AOR's own `fp/Dir.mk`
    passes; without it, 43 of 45 files fail with "thumb conditional
    instruction should be in IT block".
  - On the Thumb-2 cores:
    - **Double arithmetic** is soft on armv7e-m and armv8-m.main
      (single-precision FPU), so at32 `d*` is compared there.
    - **Float↔64-bit integer conversions** are library calls on every core,
      armv7e-m-dp included, so at32's `f2lz`/`l2f`/`d2lz`/`l2d` (and the
      unsigned forms) are compared on all three.
  - `fp/common/` holds C helpers for the slow paths: NaN, denormal and
    underflow handling.

## How AOR routines are substituted

The aim is to measure the effect of the substituted routines alone. Every
comparison program is the same program, with the same objects and the same
`libc.a`. The only difference is an AOR archive placed before `libc.a` and
libgcc on the link line.

- **Archives (P4.02):** `build/<board>/aor/` gets three archives:
  - `libaor-string.a`: only the routines that run on that core, renamed to
    the standard names;
  - `libaor-math.a`;
  - `libaor-fp.a`: Arm only, renamed to the `__aeabi_*` and `__*sf2`/`__*df2`
    names.

  AOR's build is set up for Linux/glibc test tools (MPFR, `mathbench`,
  `ulp`), so a small script compiles the selected files directly with our
  toolchain and flags. This makes the selection explicit per core.
- **Flags:** every board's `CROSS_FLAGS`.
  - **math:** AOR's own `config.mk.dist` math flags at **-O3** for the speed
    comparison, and **-Os** for the size comparison (what an embedded user
    would pick), with `-std=c23 -nostdinc -isystem include`.
  - **string and fp:** assembler sources, the same for both.
- **Link order:** `<program objects> <port> libaor-*.a -Wl,--start-group libc.a
  libgcc.a -Wl,--end-group`.
  - An AOR symbol therefore wins over ours: `libc.a` holds one function per
    member (D6), so the member that defines it is never pulled.
  - **libgcc bundles.** On the Thumb-2 multilibs, libgcc groups several
    helpers in one member. For example, `_arm_addsubdf3.o` defines `dadd`,
    `dsub`, `drsub`, `i2d`, `ui2d`, `f2d`, `l2d` and `ul2d`. If AOR provides
    only some of a member's symbols, a reference to another one pulls the
    whole member and its duplicates.
  - at32 provides every RTABI soft-float entry. The comparison links with
    `-Wl,--allow-multiple-definition` (the first definition, AOR's, wins), and
    P4.02 checks every substituted symbol in the map file. A comparison whose
    map shows another definition chosen fails rather than reporting a wrong
    number.
- **Speed (P4.03):** `bench-runner compare` relinks the objects of the
  `test/bench` programs from the speed build tree with each AOR archive in
  front. It reports the AOR build next to rtoskit's size and speed flavours.
  The other cases of the same programs check that only the substituted
  functions moved.
- **Soft-float cases:** `test/bench` has no soft-float cases. Every math case
  includes them, but none isolates them. P4.03 adds a `softfloat` bench
  program:
  - float and double add, mul, div and compare;
  - `f2d`/`d2f`;
  - the 32- and 64-bit integer conversions.

  Each operation goes through a `volatile` so that GCC emits the library
  call. The program is built and run for every board, like the others, so our
  own helpers are measured from then on.
- **Size (P4.04):**
  - **Per routine:** `.text + .rodata + .data` of each AOR member that would be
    linked, its data tables included (for example `exp2f_data.o`,
    `sincosf_data.o`, the `math_err*.o` helpers, and the libgcc soft-double
    members that the float functions pull on single-precision cores, which a
    program pays for). This is compared with our members' figures from
    `size-report`.
  - **Whole images:** the `test/size` reference programs, relinked the same
    way from the release (size) build.
- **Accuracy (P4.05):** AOR's math built for the host (x86_64, which has
  hardware double, so it measures the algorithm), against our x86_64 build.
  - **Float:** all 2^32 inputs.
  - **Double:** random and hard-case arguments.
  - **Reference:** mpmath at 200 bits, as our earlier exhaustive checks used.
  - **Arm:** the Arm builds of the float functions get a sampled check under
    qemu, so the soft-double path is covered too.

## Edge cases and caveats to carry into P4.02–P4.06

- **Clean-room:** the AOR checkout stays outside the source tree
  (`local/aor-<rev>/`), and nothing under `libc/` includes or links it.
- **NaN conventions:** AOR's fp follows Arm's. Ours follows the soft
  `<fenv.h>`, which matters only for which NaN payload survives. Any
  correctness check in the comparison compares values, not NaN payloads.
- **Exception flags:** AOR's fp raises none and ignores the rounding mode.
  - So `fenv`-dependent tests (libc-test `math/*` in non-nearest modes) cannot
    run on the AOR-substituted builds.
  - The comparison runs only the bench and size programs. It never claims
    conformance for them.
- **x86_64:** only the math comparison applies.
- **Armv6-M strings:** only `strcmp`. Our `memcpy`/`memset` speed kernels have
  no AOR counterpart there.

## Test vectors for the smoke test (P4.02)

Each substituted routine is called once in a program run under
`TARGET_RUNNER`:

- `memchr` (found and not found), `memset` (unaligned, 0 and 33 bytes),
  `strlen` (0, 1, 15 and 64 bytes), `strcmp` (equal, differing at byte 0 and at
  byte 13);
- `expf(1)`, `logf(2)`, `sinf(1)`, `powf(2, 0.5)`, `exp(1)`, `log(2)`,
  `pow(2, 0.5)`;
- `1.5f * 3` (`fmul`), `1 / 3.0` (`ddiv`) and `(long long)-1.5` (`d2lz`),
  through `volatile`.

The program checks that each result is within 1 ulp of a constant. It is a
link and run check, not an accuracy test.

## The build (P4.02)

`make aor` fetches AOR into `local/aor-<rev>/` (`make setup-aor`, a shallow
`git fetch` checked against the commit ID). For every board it then runs
`tools/scripts/aor-build build` and `tools/scripts/aor-build smoke` against the
release build.

Archives in `build/<board>/aor/`, with the global symbols each defines after
renaming (listed in `aor.json`):

| Board | `libaor-string.a` | `libaor-math.a` / `libaor-math-size.a` | `libaor-fp.a` |
|---|---|---|---|
| linux-x86_64 | — | 41 | — |
| linux-armv6-m, linux-armv6s-m | `strcmp` | 41 | 32 (`fp/armv6-m` + `fp/common`) |
| linux-armv7e-m, -dp, linux-armv8-m.main | `memchr`, `memset`, `strlen` | 41 | 56 (`fp/at32` + `fp/common`) |

The 41 math symbols are:

- the public functions;
- `sincosf`;
- AOR's internal tables and error helpers, which are named `arm_math_*`.
  These are not reserved names, which is harmless in a measurement build.

Three things only the build showed:

- **`memchr.S` and `memset.S` force `.arch armv7-a`.** The objects then carry
  the A profile, and the linker refuses to mix them with ours ("conflicting
  architecture profiles A/M").
  - The build copies those two files and drops the directive. The assembler
    then accepts and checks them against the board's own `-march`, and both
    assemble unchanged for armv7e-m and armv8-m.main.
  - This is the only check that the instructions are M-profile ones:
    `qemu-arm -cpu max` emulates an A-profile core, so it would run A-only
    Thumb-2 code without complaint.
- **Leading the link group is not enough.** On Armv6-M, libgcc's
  `_fixunsdfsi.o`, pulled in by `libc.a`, calls `__aeabi_dcmpge`. ld resolved
  that call from libgcc itself, in the same archive scan, so the program held
  libgcc's comparisons.
  - `aor-build`'s `link()` now forces (`-u`) every AOR symbol the program
    contains, and relinks until that set stops growing. The first archive in
    the group, AOR's, then defines each one.
  - Duplicates that libgcc members bring along are ignored
    (`--allow-multiple-definition`, the first definition wins).
  - `check_substitution()` reads the map file's input sections, and fails a
    link in which a provided symbol's address is outside AOR's code. It caught
    this case before the fix.
- **Naming bug in the first version.** The float GNU three-way compares came
  out as `__leff2`/`__geff2`; they are now `__lesf2`/`__gesf2`.

**Smoke test:** `tools/aor/smoke.c` calls each substituted routine once (the
vectors above) and passes on all six boards. On armv7e-m it links 45 AOR
symbols: the string functions, the math functions with their tables, and the
soft double helpers. On armv7e-m-dp only `__aeabi_d2lz` and `__aeabi_l2d` of
the fp set are needed.
