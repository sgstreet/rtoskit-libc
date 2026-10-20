# Research: a single-precision libm layer and the exact float functions

Step: P2.05b. Decision: PLAN D56. Measurements: `tools/scripts/size-report`
(D51), release build. The new reference program is `test/size/float-exact`.

## Sources reviewed

- K. Packard, "Picolibc Without Double"
  (<https://www.keithp.com/blogs/picolibc-float/>) and the LCA 2020
  picolibc talk (<https://www.keithp.com/documents/lca2020-picolibc.pdf>):
  newlib's libm has two generations of `float` code:
  - the older SunPro-derived functions keep `float` intermediates;
  - the 2018 Arm-contributed ones compute in `double`.

  picolibc picks the first on targets without double-precision hardware
  (`-Dnewlib-obsolete-math`). The reasons are the ones this step measures:
  ROM, and speed on single-precision FPUs.
- GCC `-Wdouble-promotion`:
  - forum discussions (Nordic DevZone,
    <https://devzone.nordicsemi.com/f/nordic-q-a/45241/nrf52-fpu-single-precision-floating-point---gcc-compiler-flags/177839>;
    mikrocontroller.net, <https://www.mikrocontroller.net/topic/514803>)
    say the warning is incomplete in GCC, so the disassembly or symbol
    table must be checked;
  - `-fsingle-precision-constant` is often paired with it.
- libgcc for `thumb/v7e-m+fp/hard`, `v8-m.main+fp/hard` and `v6-m/nofp`
  (`nm`): `__aeabi_f2ulz` (`_fixunssfdi.o`) converts through
  `__aeabi_f2d`, `__aeabi_dmul`, `__aeabi_dsub` and `__aeabi_d2uiz`. A
  `(long long)` conversion of a `float` therefore links soft double
  arithmetic even on an FPU core, and on armv6-m `__aeabi_l2f` does too.
- C23 7.12.6–7.12.11 and Annex F (F.10.6: `ceil`, `floor`, `round`,
  `roundeven` and `trunc` do not raise inexact; `rint` does and
  `nearbyint` does not), 7.3.9 (`creal`, `cimag`, `conj`, `cproj`).
- Our own double implementations (D45–D49), whose structure the `float`
  versions follow at 32 bits:
  - bit-mask rounding;
  - long division on the significands for `fmod`/`remainder`/`remquo`;
  - the 2^52 trick for `rint`.

## Design

- **Layer `libc/math/sp`** (D46 mechanism):
  - Listed first in `LIBC_MATH_LAYERS` on every Arm core: armv6-m,
    armv6s-m, armv7e-m, armv7e-m-dp and armv8-m.main.
  - Its `subdir.mk` drops every file that a later layer of the core
    provides in an instruction: the FPv5 `VRINT` rounding on armv8-m.main
    and armv7e-m-dp, and `fabsf`/`sqrtf`/`fmaf` everywhere.
  - The generic `libc/math` already drops whatever any layer provides.
- **Enforcement:**
  - `sp.mk` compiles with `-Wdouble-promotion` (as `-Werror`).
  - After compiling, a `sp.checked` rule runs `nm` over the layer's objects
    and fails the build on any undefined `__aeabi_d*`, `__aeabi_f2d`,
    `__aeabi_f2[u]lz`, `__aeabi_[u]l2[df]` or `__aeabi_[u]i2d`.
  - `-fsingle-precision-constant` was not adopted: it changes the meaning
    of every unsuffixed constant silently. The warning plus the symbol
    check catch the same mistakes loudly.
- **Functions (float only):**
  - `ceilf`, `floorf`, `truncf`, `roundf`, `roundevenf`: masks on the bits.
  - `rintf` adds and subtracts 2^23 in the FPU, so it uses the current
    direction and raises inexact. `nearbyintf` saves and restores the
    flags around it.
  - `__libc_remainderf`: long division on 24-bit significands. It is used
    by `fmodf`, `remainderf` and `remquof`, with the same quotient bits as
    the double core.
  - `frexpf`, `ilogbf`, `llogbf`, `logbf` read the bits. A subnormal is
    first scaled by 2^32, exactly.
  - `modff` uses `truncf`, and the fraction takes the sign of x.
  - `__libc_float_to_i64` shifts the significand. It is used by `llrintf`,
    `llroundf`, and `lrintf`/`lroundf` where `long` is 64 bits; a 32-bit
    `long` converts with VCVT or the soft single helper.
  - `__libc_fromfpf`, used by `fromfpf`, `ufromfpf`, `fromfpxf` and
    `ufromfpxf`: the float rounding functions with a `scalbnf` limit.
  - `nexttowardf`:
    - it widens x to the bits of a double exactly (normalising a subnormal
      float) and compares sign-magnitude integers, so it does no double
      arithmetic;
    - a NaN y becomes a quiet float NaN that keeps the high payload bits;
    - for equal arguments the result is y, which for zeros means y's sign.
- **Generic, every core:** `crealf`, `cimagf`, `conjf` and `cprojf` read the
  parts of a `float complex` through a union (`__libc_ref`/`__libc_imf` in
  `internal/complex.h`) instead of converting the value to `double complex`.

## Measured (code bytes, release, `float-exact`)

| Core | Before (double wrappers) | After (`sp` layer) |
|---|---:|---:|
| armv6-m | 8440 | 3908 |
| armv7e-m | 5056 | 2048 |
| armv7e-m-dp | 2002 | 1688 |
| armv8-m.main | 4552 | 1612 |

- On armv7e-m and armv8-m.main the image links no libgcc at all
  (2040 bytes before).
- `libc.a` grows by 0.8–1.5 KB per Arm core.
- The full tables are in `docs/size/<arch>.md`.

- `libc.a` grows on the cores where a float function used to be a wrapper
  of a few bytes: the float bodies are now separate code. A program that
  calls both `fmod` and `fmodf` pays for both cores.
- A program that uses `float` only no longer links the soft double adder,
  multiplier, divider and conversions. That is the D2/§1 target: small
  float programs on small cores.
- armv6-m and armv7e-m-dp were not in the original step text. Both measure
  smaller with the layer (soft single precision is far cheaper than soft
  double precision; on fpv5-dp the float bodies are smaller than wrapper
  plus double function), so the layer is used on every Arm core.

## Edge cases

- `ceilf(-0.5)` is −0, `floorf(0.5)` is +0, `roundf(±0.5)` is ±1.
  `roundevenf(±0.5)` is ±0 and `roundevenf(2.5)` is 2.
- 2^23 − 0.5 = 8388607.5 rounds to 8388608 under `rintf`/`roundevenf` and
  `roundf`. ±2^23 and up are returned unchanged.
- `rintf(−0.3)` is −0: the sign is restored after the 2^23 trick.
- `remainderf` ties: 2.5 rem 1 is 0.5, 3.5 rem 1 is −0.5, 7 rem 2 is −1.
  `remquof` returns the low quotient bits with the sign of x/y.
- `lrintf(2^31)` returns `LONG_MIN` with invalid, as on every core.
  `llrintf(−2^63)` is `LLONG_MIN` and is exact.
- `nexttowardf(0, −0)` is −0. `nexttowardf(0, −1)` is −2^−149 (with
  underflow). `nexttowardf(FLT_MAX, ∞)` is ∞ (with overflow).
- `frexpf` of the smallest subnormal gives 0.5 · 2^−148.
- `cprojf(NaN + ∞i)` is ∞ + 0i, and `cprojf(x − ∞i)` is ∞ − 0i.

## Tests

- New `test/unit/float_exact`, on every board. Each function's result is
  compared, bit for bit (any NaN for any NaN), with the double function on
  the widened argument; every result is exactly representable. The
  arguments are 40 000 values: special values, the rounding boundaries,
  and random fractions at every exponent.
  - The rounding functions; `l(l)rintf` and `l(l)roundf`.
  - `(u)fromfp(x)f` for every rounding direction and widths 0 … `UINT_MAX`.
  - `frexpf`, `ilogbf`, `llogbf`, `logbf` and `modff`.
  - `fmodf`, `remainderf` and `remquof` (quotient included), with random
    divisors near the dividend and anywhere, plus 40 000 exact ties.
  - `nexttowardf` toward y equal to x, just beside x, and random; plus the
    signed zeros and the limits.
  - The four complex accessors on every pair of special parts.
- Existing tests: `test/unit/libm*`, the fenv tests (flags on fpv5-dp,
  where the layer now runs under real exception flags and rounding
  directions), and libc-test 219/219 on every board.
- Fault injection: 11 mutants, every one caught. One survived at first
  (the ties-to-even test in `__libc_remainderf`); the tie loop was added
  for it.
