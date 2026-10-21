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

## Found in the size comparison (P4.04)

AOR's `strcmp-armv6m.S` and `strlen-armv6t2.S` keep EHABI unwind directives.

- **Effect:** their `.ARM.exidx` entries refer to `__aeabi_unwind_cpp_pr0`, so
  a program that links either gets libgcc's unwinder: about 3.6 KB for a 136-
  or 216-byte routine.
- **How it is reported:** `docs/size/phase-4.md` reports AOR as it builds and
  names the effect, rather than stripping the tables (D95).

## Accuracy (P4.05)

`tools/scripts/aor-accuracy` (needs `libmpfr-dev` on the host). The raw
results are in `docs/research/aor-accuracy-host.txt` and
`docs/research/aor-accuracy-arm.txt`; `aor-accuracy markdown <file>` prints
the tables below.

### Method

**Host program.** `tools/aor/accuracy.c` is built for x86_64 with glibc,
MPFR and four threads. It calls our x86_64 functions as `rt_*`: `libc.a` with
every symbol prefixed, so it stays whole beside glibc. It calls AOR's
functions as `aor_*`: their entry points renamed, with `fma` from glibc. The
reference is MPFR at 200 bits.

**What counts as correctly rounded.** The correctly rounded result is
computed by MPFR straight into the format and the mode, subnormals included.
Rounding the 200-bit value instead would get `exp(2^-842)` wrong in an upward
mode, which a first version of the harness did.

**Error.** Measured in ulps of the exact value's binade, using the subnormal
ulp below the normal range. Results beyond the format's range count only as
correctly rounded or not.

**Coverage.**

- **Float functions:** every one of the 2^32 arguments. Where the two
  libraries' results differ, both are checked against MPFR. Where they agree,
  1 argument in 16 is checked: an agreeing result is either correct or wrong
  in both libraries, and the sample estimates the latter.
- **`powf` and the double functions:** 2^26 and 2^24 sampled arguments, every
  one checked. Half of them are uniform over the function's useful domain and
  half are spread over the binades.
- **Directed rounding modes:** 2^20 samples per mode. Only the library calls
  run in that mode; MPFR and the argument generator run in round to nearest.
- **Special arguments:** each library's result and the exceptions it raises.

**Arm check.** `tools/aor/arm-sample.c` runs under qemu on armv6-m, armv7e-m
and armv7e-m-dp: 16,384 sampled arguments per function, for our size flavour,
our speed flavour, and AOR (string, math `-O3` and fp). The host measures its
records. This covers AOR's soft double, our float-float layer (`sp-nearest`)
and our Armv6-M fixed-point paths.

### Results on x86_64

For the exhaustive rows, "not correctly rounded" counts every misrounded
result among the arguments where the two libraries differ, plus those found
in the 1-in-16 sample where they agree, as a share of all 2^32 arguments.

#### Round to nearest

| Function | Arguments | Checked | Results that differ | rtoskit max ulp | rtoskit not correctly rounded | AOR max ulp | AOR not correctly rounded |
|---|---|---:|---:|---:|---:|---:|---:|
| `expf` | all 2^32 | 215,844,847 | 170,646 | 0.5000 | 0 | 0.5016 | 170646 (0.00397 %) |
| `exp2f` | all 2^32 | 216,164,354 | 168,364 | 0.5000 | 2 (4.66e-08 %) | 0.5016 | 168362 (0.00392 %) |
| `logf` | all 2^32 | 134,084,024 | 416,909 | 0.5000 | 3 (6.98e-08 %) | 0.8177 | 416906 (0.00971 %) |
| `log2f` | all 2^32 | 133,987,074 | 313,550 | 0.5000 | 0 | 0.7518 | 313550 (0.0073 %) |
| `log10f` | all 2^32 | 134,084,584 | 417,552 | 0.5000 | 1 (2.33e-08 %) | 0.8197 | 417551 (0.00972 %) |
| `sinf` | all 2^32 | 294,913,660 | 29,362,802 | 0.5000 | 0 | 0.5607 | 29362802 (0.684 %) |
| `cosf` | all 2^32 | 293,831,862 | 28,209,648 | 0.5000 | 2 (4.66e-08 %) | 0.7758 | 28209646 (0.657 %) |
| `erff` | all 2^32 | 425,327,258 | 168,467,910 | 0.5000 | 0 | 1.0900 | 168467910 (3.92 %) |
| `powf` | 67,108,864 sampled | 67,108,864 | 15,625 | 0.5000 | 0 | 0.5059 | 15625 (0.0233 %) |
| `exp` | 16,777,216 sampled | 16,777,216 | 7,961 | 0.5000 | 26 (0.000155 %) | 0.5082 | 7973 (0.0475 %) |
| `exp2` | 16,777,216 sampled | 16,777,216 | 9,996 | 0.5068 | 3805 (0.0227 %) | 0.5084 | 8103 (0.0483 %) |
| `exp10` | 16,777,216 sampled | 16,777,216 | 10,775 | 0.5061 | 3828 (0.0228 %) | 0.5093 | 8955 (0.0534 %) |
| `log` | 16,777,216 sampled | 16,777,216 | 42 | 0.5000 | 0 | 0.5098 | 42 (0.00025 %) |
| `log2` | 16,777,216 sampled | 16,777,216 | 97 | 0.5000 | 0 | 0.5170 | 97 (0.000578 %) |
| `erf` | 16,777,216 sampled | 16,777,216 | 1,631,640 | 0.5300 | 1151 (0.00686 %) | 0.8871 | 1631801 (9.73 %) |
| `pow` | 16,777,216 sampled | 16,777,216 | 4,969 | 0.5051 | 1291 (0.00769 %) | 0.5084 | 4436 (0.0264 %) |

#### Directed rounding modes

Maximum ulp error and the share not correctly rounded in that mode (rtoskit / AOR).

| Function | upward | downward | toward zero |
|---|---|---|---|
| `expf` | 1.000 (0 %) / 2.000 (36 %) | 1.000 (0 %) / 1.120 (1.1 %) | 1.000 (0 %) / 1.120 (1.1 %) |
| `exp2f` | 1.000 (0 %) / 2.000 (36 %) | 1.000 (0 %) / 1.120 (1.1 %) | 1.000 (0 %) / 1.120 (1.1 %) |
| `logf` | 1.000 (0 %) / 1.248 (0.01 %) | 1.000 (0 %) / 1.198 (0.0098 %) | 1.000 (0 %) / 1.201 (0.01 %) |
| `log2f` | 1.000 (0 %) / 1.201 (0.0092 %) | 1.000 (0 %) / 1.215 (0.0082 %) | 1.000 (0 %) / 1.161 (0.0072 %) |
| `log10f` | 1.000 (0 %) / 1.126 (0.011 %) | 1.000 (0 %) / 1.135 (0.012 %) | 1.000 (0 %) / 1.109 (0.011 %) |
| `sinf` | 1.000 (0 %) / 1.060 (22 %) | 1.000 (0 %) / 1.060 (22 %) | 1.000 (0 %) / 1.060 (43 %) |
| `cosf` | 1.000 (0 %) / 1.059 (0.72 %) | 1.000 (0 %) / 1.060 (43 %) | 1.000 (0 %) / 1.060 (43 %) |
| `erff` | 1.000 (0 %) / 1.553 (12 %) | 1.000 (0 %) / 1.619 (14 %) | 1.000 (0 %) / 1.581 (19 %) |
| `powf` | 1.000 (0 %) / 2.000 (35 %) | 1.000 (0 %) / 1.120 (0.78 %) | 1.000 (0 %) / 1.120 (0.79 %) |
| `exp` | 1.000 (9.5e-05 %) / 1.012 (0.93 %) | 1.000 (0.00019 %) / 1.240 (1.5 %) | 1.000 (0.00019 %) / 1.240 (1.5 %) |
| `exp2` | 1.010 (0.046 %) / 1.012 (0.9 %) | 1.011 (0.042 %) / 1.271 (1.7 %) | 1.009 (0.048 %) / 1.275 (1.7 %) |
| `exp10` | 1.012 (0.048 %) / 1.009 (1.7 %) | 1.009 (0.045 %) / 1.185 (1.2 %) | 1.008 (0.044 %) / 1.184 (1.2 %) |
| `log` | 1.000 (0 %) / 1.004 (0.00076 %) | 1.000 (0 %) / 1.006 (0.0014 %) | 1.000 (0 %) / 1.006 (0.002 %) |
| `log2` | 1.000 (0 %) / 1.026 (0.052 %) | 1.000 (0 %) / 1.119 (0.05 %) | 1.000 (0 %) / 1.020 (0.047 %) |
| `erf` | 1.010 (0.0061 %) / 1.523 (11 %) | 1.016 (0.0055 %) / 1.562 (11 %) | 1.006 (0.0064 %) / 1.470 (11 %) |
| `pow` | 1.010 (0.016 %) / 1.014 (0.55 %) | 1.009 (0.019 %) / 1.246 (0.93 %) | 1.010 (0.02 %) / 1.248 (0.93 %) |

#### Special arguments

Result and exceptions raised (I invalid, Z divide-by-zero, O overflow, U underflow, X inexact); rows where the libraries differ are marked.

| Function | x | y | rtoskit | AOR | |
|---|---|---|---|---|---|
| `expf` | `0x0p+0` |  | `0x1p+0` - | `0x1p+0` - |  |
| `expf` | `-0x0p+0` |  | `0x1p+0` - | `0x1p+0` - |  |
| `expf` | `inf` |  | `inf` - | `inf` - |  |
| `expf` | `-inf` |  | `0x0p+0` - | `0x0p+0` - |  |
| `expf` | `nan` |  | `nan` - | `nan` - |  |
| `expf` | `0x1.64p+6` |  | `inf` OX | `inf` OX |  |
| `expf` | `-0x1.ap+6` |  | `0x0p+0` UX | `0x0p+0` UX |  |
| `expf` | `-0x1.9fe368p+6` |  | `0x1p-149` UX | `0x1p-149` UX |  |
| `logf` | `0x0p+0` |  | `-inf` Z | `-inf` Z |  |
| `logf` | `-0x0p+0` |  | `-inf` Z | `-inf` Z |  |
| `logf` | `-0x1p+0` |  | `-nan` I | `-nan` I |  |
| `logf` | `0x1p+0` |  | `0x0p+0` - | `0x0p+0` - |  |
| `logf` | `inf` |  | `inf` - | `inf` - |  |
| `logf` | `-inf` |  | `-nan` I | `-nan` I |  |
| `logf` | `nan` |  | `nan` - | `nan` - |  |
| `logf` | `0x1p-149` |  | `-0x1.9d1dap+6` X | `-0x1.9d1dap+6` X |  |
| `sinf` | `0x0p+0` |  | `0x0p+0` - | `0x0p+0` - |  |
| `sinf` | `-0x0p+0` |  | `-0x0p+0` - | `-0x0p+0` - |  |
| `sinf` | `inf` |  | `-nan` I | `-nan` I |  |
| `sinf` | `nan` |  | `nan` - | `nan` - |  |
| `sinf` | `0x1p-149` |  | `0x1p-149` UX | `0x1p-149` UX |  |
| `sinf` | `0x1p+127` |  | `0x1.3f2c62p-1` X | `0x1.3f2c62p-1` X |  |
| `cosf` | `inf` |  | `-nan` I | `-nan` I |  |
| `cosf` | `0x0p+0` |  | `0x1p+0` - | `0x1p+0` - |  |
| `erff` | `0x0p+0` |  | `0x0p+0` - | `0x0p+0` - |  |
| `erff` | `-0x0p+0` |  | `-0x0p+0` - | `-0x0p+0` - |  |
| `erff` | `inf` |  | `0x1p+0` - | `0x1p+0` - |  |
| `erff` | `-inf` |  | `-0x1p+0` - | `-0x1p+0` - |  |
| `erff` | `0x1p-149` |  | `0x1p-149` UX | `0x1p-149` UX |  |
| `powf` | `0x0p+0` | `-0x1p+0` | `inf` Z | `inf` Z |  |
| `powf` | `-0x0p+0` | `-0x1p+0` | `-inf` Z | `-inf` Z |  |
| `powf` | `-0x1p+0` | `0x1p-1` | `-nan` I | `-nan` I |  |
| `powf` | `-0x1p+0` | `inf` | `0x1p+0` - | `0x1p+0` - |  |
| `powf` | `0x1p+0` | `nan` | `0x1p+0` - | `0x1p+0` - |  |
| `powf` | `nan` | `0x0p+0` | `0x1p+0` - | `0x1p+0` - |  |
| `powf` | `0x1p+1` | `0x1p+7` | `inf` OX | `inf` OX |  |
| `powf` | `0x1p+1` | `-0x1.2cp+7` | `0x0p+0` UX | `0x0p+0` UX |  |
| `powf` | `-0x1p+1` | `0x1.8p+1` | `-0x1p+3` - | `-0x1p+3` - |  |
| `powf` | `inf` | `-0x1p-1` | `0x0p+0` - | `0x0p+0` - |  |
| `exp` | `0x0p+0` |  | `0x1p+0` - | `0x1p+0` - |  |
| `exp` | `inf` |  | `inf` - | `inf` - |  |
| `exp` | `-inf` |  | `0x0p+0` - | `0x0p+0` - |  |
| `exp` | `nan` |  | `nan` - | `nan` - |  |
| `exp` | `0x1.63p+9` |  | `inf` OX | `inf` OX |  |
| `exp` | `-0x1.75p+9` |  | `0x0p+0` UX | `0x0p+0` UX |  |
| `exp` | `-0x1.6232bdd7abcd2p+9` |  | `0x1.000000000007cp-1022` X | `0x1.000000000007cp-1022` X |  |
| `log` | `0x0p+0` |  | `-inf` Z | `-inf` Z |  |
| `log` | `-0x1p+0` |  | `-nan` I | `-nan` I |  |
| `log` | `0x1p+0` |  | `0x0p+0` - | `0x0p+0` - |  |
| `log` | `inf` |  | `inf` - | `inf` - |  |
| `log` | `0x0.0000000000001p-1022` |  | `-0x1.74385446d71c3p+9` X | `-0x1.74385446d71c3p+9` X |  |
| `erf` | `0x0p+0` |  | `0x0p+0` - | `0x0p+0` - |  |
| `erf` | `-0x0p+0` |  | `-0x0p+0` - | `-0x0p+0` - |  |
| `erf` | `inf` |  | `0x1p+0` - | `0x1p+0` - |  |
| `erf` | `0x0.0000000000001p-1022` |  | `0x0.0000000000001p-1022` UX | `0x0.0000000000001p-1022` UX |  |
| `pow` | `0x0p+0` | `-0x1p+0` | `inf` Z | `inf` Z |  |
| `pow` | `-0x1p+0` | `0x1p-1` | `-nan` I | `-nan` I |  |
| `pow` | `0x1p+1` | `0x1p+10` | `inf` OX | `inf` OX |  |
| `pow` | `0x1p+1` | `-0x1.0ccp+10` | `0x0p+0` UX | `0x0p+0` UX |  |
| `pow` | `-0x1p+1` | `0x1.8p+1` | `-0x1p+3` - | `-0x1p+3` X | **differs** |
| `pow` | `0x1p+0` | `nan` | `0x1p+0` - | `0x1p+0` - |  |
| `pow` | `nan` | `0x0p+0` | `0x1p+0` - | `0x1p+0` - |  |

### Results on the Arm targets

16,384 sampled arguments per function, round to nearest: maximum ulp error / results not correctly rounded.

| Function | armv6-m/size | armv6-m/speed | armv6-m/aor | armv7e-m/size | armv7e-m/speed | armv7e-m/aor | armv7e-m-dp/size | armv7e-m-dp/speed | armv7e-m-dp/aor |
|---|---|---|---|---|---|---|---|---|---|
| `expf` | 0.500 / 0 | 0.500 / 0 | 0.501 / 4 | 0.500 / 0 | 0.500 / 0 | 0.501 / 4 | 0.500 / 0 | 0.500 / 0 | 0.501 / 4 |
| `exp2f` | 0.500 / 0 | 0.500 / 0 | 0.501 / 5 | 0.500 / 0 | 0.500 / 0 | 0.501 / 5 | 0.500 / 0 | 0.500 / 0 | 0.501 / 5 |
| `logf` | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 |
| `log2f` | 0.500 / 0 | 0.500 / 0 | 0.541 / 1 | 0.500 / 0 | 0.500 / 0 | 0.541 / 1 | 0.500 / 0 | 0.500 / 0 | 0.541 / 1 |
| `log10f` | 0.500 / 0 | 0.500 / 0 | 0.510 / 6 | 0.500 / 0 | 0.500 / 0 | 0.510 / 6 | 0.500 / 0 | 0.500 / 0 | 0.510 / 6 |
| `sinf` | 0.500 / 0 | 0.500 / 0 | 0.559 / 110 | 0.500 / 0 | 0.500 / 0 | 0.559 / 110 | 0.500 / 0 | 0.500 / 0 | 0.559 / 110 |
| `cosf` | 0.500 / 0 | 0.500 / 0 | 0.558 / 108 | 0.500 / 0 | 0.500 / 0 | 0.558 / 108 | 0.500 / 0 | 0.500 / 0 | 0.558 / 108 |
| `erff` | 0.500 / 0 | 0.500 / 0 | 0.887 / 1298 | 0.500 / 0 | 0.500 / 0 | 0.887 / 1063 | 0.500 / 0 | 0.500 / 0 | 0.887 / 1063 |
| `powf` | 0.500 / 0 | 0.500 / 0 | 0.504 / 6 | 0.500 / 0 | 0.500 / 0 | 0.504 / 6 | 0.500 / 0 | 0.500 / 0 | 0.504 / 6 |
| `exp` | 0.500 / 0 | 0.500 / 0 | 0.502 / 9 | 0.500 / 0 | 0.500 / 0 | 0.502 / 9 | 0.500 / 0 | 0.500 / 0 | 0.502 / 6 |
| `exp2` | 0.501 / 4 | 0.501 / 4 | 0.502 / 8 | 0.501 / 4 | 0.501 / 4 | 0.502 / 8 | 0.501 / 4 | 0.501 / 4 | 0.501 / 9 |
| `log` | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 |
| `log2` | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 | 0.500 / 0 |
| `erf` | 0.500 / 0 | 0.500 / 0 | 0.837 / 1560 | 0.500 / 0 | 0.500 / 0 | 0.837 / 1560 | 0.500 / 0 | 0.500 / 0 | 0.837 / 1541 |
| `pow` | 0.500 / 0 | 0.500 / 0 | 0.502 / 6 | 0.500 / 0 | 0.500 / 0 | 0.502 / 6 | 0.500 / 0 | 0.500 / 0 | 0.502 / 6 |

### What the accuracy comparison shows

**Our float functions on x86_64.**

- They are correctly rounded to nearest for all but 8 of the 2^32 arguments,
  which falls short of D47's claim.
- The 8 arguments are all in the set where the two libraries differ. The
  1-in-16 sample of agreeing arguments found no more, and the per-function
  counts match the exhaustive run exactly:

  | Function | Argument | Our result |
  |---|---|---|
  | `exp2f` | `0x1.853a6ep-9` | `0x1.008708p+0` |
  | `exp2f` | `-0x1.e7526ep-6` | `0x1.f58d6p-1` |
  | `logf` | `0x1.b121a6p+76` | `0x1.a9a3fp+5` |
  | `logf` | `0x1.6351d8p+95` | `0x1.08b51p+6` |
  | `logf` | `0x1.827a74p-7` | `-0x1.1c2b2p+2` |
  | `log10f` | `0x1.fddcf4p-98` | `-0x1.d33a44p+4` |
  | `cosf` | `±0x1.2b9622p+67` | `0x1.f0285cp-1` |

- These cores compute in double and round to float. The double result is
  then within 2^-29 float ulps of a float halfway point, so the second
  rounding goes the wrong way.

**Directed rounding modes.** Ours are correctly rounded in every mode for
float (0 % misrounded), and within 1.02 ulp for double.

**AOR's float functions.**

- To nearest: 0.004–0.01 % misrounded for the exp and log family, 0.66–0.68 %
  for `sinf`/`cosf`, and 3.9 % for `erff`, with a worst case of 1.09 ulp
  (`erff`).
- In the directed modes, `expf`, `exp2f` and `powf` reach 2 ulp upward, and
  35 % of results are misrounded.

**Double functions.**

- Both libraries stay below 0.53 ulp, except AOR's `erf` at 0.89 ulp (9.7 %
  misrounded).
- Ours misround up to 0.023 % (`exp2`, `exp10`) and AOR's up to 0.053 %.

**Special arguments.**

- Both libraries return the same values and raise the same exceptions (by
  arithmetic) for 61 of the 62 cases.
- The exception: AOR's `pow(-2, 3)` raises a spurious inexact.
- AOR's Arm soft-float raises no flags at all, so on the cores without
  hardware double its flags are lost. This is not measured here; see the
  caveats.

**On Arm.**

- **Ours:** no misrounded result in 16,384 samples per function, in either
  flavour on any of the three cores. The Armv6-M speed flavour's fixed-point
  paths are included.
- **AOR:** its results are the same on every core, which is expected because
  soft double is exact IEEE. They show the same misrounding pattern as on
  x86_64 (`erff` 6.5–7.9 %, `erf` 9.5 %).
