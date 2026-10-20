# Research: soft-float helpers in libm (armv7e-m-dp and armv6-m)

Step: P2.05a. Decision: PLAN D55. Measurements: `tools/scripts/size-report`
(D51), release build, and per-member link maps of the reference programs.

## Sources reviewed

- Arm, *Run-time ABI for the Arm Architecture* (RTABI), 4.1.2:
  - the floating-point helpers `__aeabi_dadd`, `__aeabi_dsub` (x − y) and
    `__aeabi_drsub` (y − x);
  - the conversions `__aeabi_l2d`, `__aeabi_ul2d`, `__aeabi_d2lz` and
    `__aeabi_d2ulz`;
  - their declarations in NetBSD's `sys/arch/arm/include/aeabi.h`
    (<https://src.rcs.uwaterloo.ca/xref/netbsd-src/sys/arch/arm/include/aeabi.h>).
- GCC mailing list, "Code size issues on FP-emulation on libgcc compared to
  LLVM's compiler_rt" (<https://gcc.gnu.org/pipermail/gcc/2015-June/217966.html>):
  - libgcc's soft-fp favours speed and generality;
  - `__addsf3` and `__subsf3` each inline the whole add/subtract logic;
  - a smaller library would share the addition and subtraction code.
- The libgcc patch that split Thumb-1 soft-float multiplication from
  division (<https://gcc.gnu.org/ml/gcc-patches/2015-05/msg00997.html>): the
  same "link only what is used" idea.
- LLVM compiler-rt `aeabi_drsub.c`
  (<https://codebrowser.dev/llvm/compiler-rt/lib/builtins/arm/aeabi_drsub.c.html>,
  description only): the reverse subtraction is a wrapper that swaps the
  operands.
- The libgcc archives of the toolchain (arm-none-eabi GCC 16.2), inspected
  with `nm`, `size` and `objdump`:
  - `thumb/v6-m/nofp`:
    - `adddf3.o` (1432 bytes) defines only `__aeabi_dadd`, and `subdf3.o`
      (1444) only `__aeabi_dsub`;
    - `addsf3.o` (748) and `subsf3.o` (764) the same for `float`;
    - `_arm_addsubdf3.o` (16 bytes) is `__aeabi_drsub`: it flips the sign
      bit and calls `__aeabi_dadd`.
  - `v7e-m+fp` and `v7e-m+dp`:
    - `_arm_addsubdf3.o` (888 bytes, the soft double addition) also holds
      `__aeabi_ul2d` and `__aeabi_l2d`, so a 64-bit integer to double
      conversion links the whole soft adder, even on fpv5-dp;
    - `__aeabi_d2lz` (44 bytes) calls `__aeabi_d2ulz` (64).
- IEEE 754-2019:
  - 5.4.1: subtraction(x, y) is x + (−y);
  - 5.5.1: negate changes only the sign bit and is not arithmetic;
  - 6.3: the sign of an exact zero sum.
- Armv7-M and Armv8-M Architecture Reference Manuals:
  - VCVT between floating point and integers handles 32-bit integers only;
  - VCVT toward zero is exact for integral values;
  - VRINTM (FPv5) rounds toward minus infinity;
  - VCVT saturates out-of-range values (NaN converts to 0) and raises
    Invalid Operation.

## Findings

| Arch | Helper pulled in | From | Cost | Fix |
|---|---|---|---:|---|
| armv7e-m-dp | `__aeabi_ul2d`, `__aeabi_l2d` (`_arm_addsubdf3.o`) | `__libc_exact_to_double`, `__libc_rem_pio2`, `getpayload`, `strtod`; `__libc_pow_integer`, `rootn`, `compoundn`, `difftime` | 888 | `__libc_u64_to_double` / `__libc_i64_to_double`: in the fpv5-dp layer, `(double)high * 2^32 + (double)low`. Both halves convert exactly, the product is exact, and the one addition rounds as the conversion does (in the current direction, raising inexact as it should) |
| armv7e-m-dp | `__aeabi_d2lz` → `__aeabi_d2ulz` | `llrint`, `llround`, `setpayload`, `setpayloadsig` | 108 | `__libc_double_to_i64`: high = VRINTM(x · 2^−32), low = x − high · 2^32. Both are exact for an integral x, and each half converts with a 32-bit VCVT |
| armv7e-m-dp | `__aeabi_ul2f` | `strtof`'s fast path | soft single adder | the significand is at most 2^24: convert through `uint32_t` |
| armv6-m, armv7e-m, armv8-m.main | soft `__aeabi_ddiv`/`fdiv` and multiplications | the special-case results `0/0`, `±1/0` and `±tiny · tiny`, inline in every caller | `divdf3.o` 1332 on armv6-m | Out of line in `libc/fenv`. Where `<fenv.h>` records exceptions (x86_64, fpv5-dp) they stay arithmetic. On the soft variant they are constants (`NaN`, `±∞`, `±0`): it records no flags and has only round to nearest (D44), so the arithmetic was dead weight |
| armv6-m, armv7e-m, armv8-m.main | soft float operations | the five `__libc_math_raise_*` helpers, inline everywhere | 2–30 bytes per site | Out of line in `libc/fenv` and empty on the soft variant |
| armv6-m | `subdf3.o` 1444, `subsf3.o` 764 | every double or float subtraction | 1444 + 764 | the `nofp` libm layer defines `__aeabi_dsub(a, b)` as `__aeabi_dadd(a, −b)` (14 bytes) and `__aeabi_fsub` the same way |

- `libc.a` is ahead of libgcc in the link group, so a program's and the
  library's references to `__aeabi_dsub` resolve to the 14-byte member.
  libgcc's own objects do not refer to it.
- The comparison helpers (`gedf2.o`/`ledf2.o`, 200 bytes each) were left
  alone. `_arm_cmpdf2.o` in libgcc refers to them, so whether a libc
  replacement wins depends on the order in which the archives are searched.
  That is fragile, and they are small.

## Rejected

- **`lrint`/`lround` as one VCVTR/VCVTA on fpv5-dp** (about −80 bytes each):
  - VCVT saturates out-of-range values (`LONG_MAX` for large positive
    values, 0 for NaN);
  - the library returns `LONG_MIN` on every architecture, which matches x86
    and is pinned by `test/unit/libm`;
  - the value is unspecified (F.10.6.5), but a uniform result across the
    targets is worth more than 80 bytes;
  - P2.05b revisits the `float` forms.
- **Keeping the inline bit-level `__libc_double_to_i64`**: each of its four
  callers grew by 50–110 bytes. One out-of-line copy (56 bytes on fpv5-dp,
  96 on armv6-m; a single CVTTSD2SI on x86_64) is smaller once two callers
  are linked.
- **Replacing libgcc's comparisons or division**: either fragile (above) or
  a rewrite of soft-fp. Hand-written Thumb-1 kernels are P2.06's subject.

## Measured effect (code bytes, release)

| Image / archive | armv6-m | armv7e-m | armv7e-m-dp | armv8-m.main |
|---|---:|---:|---:|---:|
| `libc.a` | 121640 → 120330 | 121548 → 120404 | 86636 → 85598 | 121046 → 120172 |
| `math-mix` | 30024 → 27864 | 23460 → 23256 | 13244 → 12326 | 23448 → 23244 |
| `math-mix` libgcc share | 9368 → 7412 | 2504 → 2504 | 888 → 0 | 2504 → 2504 |

- After the change, no armv7e-m-dp `libc.a` member refers to a double
  soft-float helper.
- The remaining `__aeabi_ldivmod` is in `lldiv`/`imaxdiv`, which divide
  natively by design (D54).

## Edge cases

- `__libc_u64_to_double(2^64 − 1)` is 2^64. Ties go to even: 2^53 + 1 →
  2^53, and 2^53 + 3 → 2^53 + 4. `__libc_i64_to_double(INT64_MIN)` is
  −2^63.
- `__libc_double_to_i64(−2^63)` is `INT64_MIN`. In the fpv5-dp version the
  high half is −2^31 and the low half is 0. ±0 gives 0.
- Subtraction:
  - x − x is +0 in round to nearest;
  - −0 − +0 is −0;
  - ∞ − ∞ is NaN;
  - 1 − 2^−54 ties to 1;
  - 1 − 1.5 · 2^−53 ties to the even neighbour;
  - `DBL_MAX` − (−2^970) overflows to +∞ (a tie, rounded up to even).
- Soft variant special results: `log(0)` = −∞, `log(−1)` = NaN,
  `exp(−1000)` = +0, `atanh(−1)` = −∞.

## Tests

- New `test/unit/softfp`, on every board:
  - the conversions against the compiler's own, over edge values and
    40 000 random values of every width, plus the known answers above;
  - `__libc_double_to_i64` against the compiler's conversion for 20 000
    random integral values and the edges;
  - subtraction known answers through volatile operands (the `nofp`
    helpers on armv6-m), and 20 000 exact integer differences, for `double`
    and `float`;
  - the out-of-line special results, directly and through public functions.
- Existing tests:
  - `test/unit/libm*` and `test/unit/fpu_math`;
  - the fenv tests, which check that exceptions are still raised on x86_64
    and fpv5-dp;
  - libc-test 219/219 on all six boards.
- Fault injection, each caught by `softfp`:
  - `__aeabi_dsub` without the negation;
  - `__aeabi_fsub` without the negation;
  - the fpv5-dp `i64` high half converted unsigned;
  - the `u64` low half converted signed;
  - the fpv5-dp `__libc_double_to_i64` rounding toward zero (VRINTZ);
  - the generic `__libc_double_to_i64` shifting one place too far;
  - soft `__libc_math_divbyzero` ignoring the sign;
  - soft `__libc_math_underflow` ignoring the sign.
