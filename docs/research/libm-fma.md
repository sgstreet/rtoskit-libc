# Research: float-float on cores with a fused multiply-add

Step: P3.05b. Decision: PLAN D82.

## Where the time went (P3.04 baselines, armv7e-m, instructions per call)

| Function | FPv4-SP (armv7e-m) | FPv5-DP (armv7e-m-dp) |
|---|---:|---:|
| `sinf` | 694 | 225 |
| `tanf` | 1032 | 325 |
| `expf` | 228 | 187 |
| `logf` | 234 | 193 |
| `erff` | 2539 | 438 |
| `tgammaf` | 1198 | 870 |

On the single-precision FPUs, every float function of the `sp-nearest`
layer (D57–D68) works in float-float to stay within 0.5 ulp:
- each exact product `__libc_ff_product` is Dekker's: two Veltkamp splits
  and four partial products, about 17 floating-point operations;
- FPv4-SP and FPv5-SP have `VFMA.F32`, which is unused there.

## Sources reviewed

- **T. J. Dekker, *A floating-point technique for extending the available
  precision*** (Numer. Math. 18, 1971): the split product, exact without an
  FMA when nothing overflows or underflows.
- **J.-M. Muller et al., *Handbook of Floating-Point Arithmetic*** (2nd ed.,
  2018), §4.4 and §5:
  - with an FMA, the error of a rounded product, `fma(a, b, -fl(ab))`, is
    exactly representable (2MultFMA);
  - this holds unless the product's low part falls below the normal range,
    the same condition as Dekker's.
- **CORE-MATH, glibc and ARM optimized-routines:** they use 2MultFMA where
  the target has an FMA, and split products elsewhere.
- **The C23 standard:** `fma` (7.12.13.1) is a single rounding. GCC's
  `__builtin_fmaf` is one `VFMA.F32` under `-march=armv7e-m+fp` and
  `armv8-m.main+fp`, even with `-fno-builtin`.

## Design

- **The FPU's header:** `libc/internal/fpu/<LIBC_FPU>/libc-fpu.h` states what
  the FPU computes in one instruction. For now that is
  `constexpr bool __libc_fpu_fmaf`, true on FPv4-SP, FPv5-SP and FPv5-DP,
  false for `nofp` and x86-64 (SSE2, D81).
  - The toolchain files put that directory on `CPPFLAGS`, so the library,
    the tests that compile library sources, and the float accuracy test
    all see the right FPU.
  - x86-64 now exports `LIBC_FPU := sse2`.
  - A header chosen by directory needs no `#if` (D22).
- **The product:** `__libc_ff_product(a, b)` returns
  `{ a·b, fma(a, b, -a·b) }` when `__libc_fpu_fmaf` is set, else Dekker's
  form. The constant folds the other branch away. Both forms give the
  same exact pair under the same conditions, so the layer's results do not
  change by a bit, and the accuracy work of D57–D68 stands.

### Measured and not taken

- **The reduction's `scalbnf` calls** (`__libc_rem_pio2f`, D60):
  - Building the scale 2^(top − 23 − shift) from its exponent bits is
    safe: a host program walked every float from 2^-6 up with the same
    160-bit arithmetic, and the smallest |d| is 2^-30 (at 0x4ea3e87f).
  - It saved 21 instructions per `sinf` on FPv4-SP but grew the member by
    8–12 bytes on Armv6-M, which the size budget does not allow, so it was
    dropped.

- **A Cody–Waite reduction in float** for moderate |x|: the reduction is
  more than half of `sinf` (241 instructions below 2^-6, 543 just above).
  - Without the exact integer arithmetic, the terms that cancel near a
    multiple of π/32 leave float-float sums with about 2^-22 relative
    error once |d| is near its minimum of 2^-30. That is far outside the
    layer's 2^-40 budget.
  - D60 chose the integer reduction for this reason, and it stays.
- **`-O2` on the layer** in the speed flavour:
  - `sinf` −10 % and `erff` −10 % more, but `expf` and `logf` 5 % slower;
  - `math-float` on Armv6-M over the speed budget;
  - on the reduction alone, 2 instructions.

## Tests

- **`test/unit/softfp`:** 20,000 random products with factors in
  [2^-30, 2^30), checked to be exact against double. Each board runs the
  form it compiles: the FMA form on FPv4-SP, FPv5-SP and FPv5-DP, Dekker's
  elsewhere.
- **`test/unit/float_accuracy` and `complex_float`:** still run the whole
  layer on every board and build type, and natively on x86-64 (the Dekker
  form).
