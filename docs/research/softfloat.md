# Research: integer float arithmetic on Armv6-M

Step: P3.05e. Decision: PLAN D85 (answers Q14 in part).

## Where the time went (P3.04 speed baseline, armv6-m, instructions per call)

| Function | Instructions |
|---|---:|
| `sqrtf` | 2036 |
| `expf` | 11458 |
| `logf` | 12864 |
| `sinf` | 18996 |
| `erff` | 151747 |

Armv6-M has no FPU. Every float operation in the `sp-nearest` layer (D57–D68) is a call to a soft-float helper:
- libgcc's `__aeabi_fadd`, `__aeabi_fmul` and `__aeabi_fdiv`;
- our `__aeabi_fsub` (D55).

The float-float steps make about ten calls per exact product (Dekker, D82) and as many again per compensated sum. `sqrtf` is computed in integers (`sp` layer, D59), but through the shared 64-bit `__libc_isqrt`, a conversion to float and `scalbnf`.

## Sources reviewed

- **IEEE 754-2019**, §4.3 (rounding) and §5.4.1 (the arithmetic operations): one correct rounding of the exact result, with the special cases for NaN, infinity and zero.
- **J. R. Hauser, *Berkeley SoftFloat* (release 3)**
  (<http://www.jhauser.us/arithmetic/SoftFloat.html>):
  - the operations are written in integers;
  - the significands are kept with extra low bits (a round bit and a sticky bit) and rounded once, in a shared `roundPackToF32`;
  - its division uses a reciprocal estimate, refined and then corrected.
- **libgcc's soft-fp** (the `__aeabi_f*` helpers that GCC links on Armv6-M): generic C over the same unpack, operate, round and pack stages. Here its `__aeabi_fmul` is 504 bytes and `__aeabi_fdiv` 512.
- **J.-M. Muller et al., *Handbook of Floating-Point Arithmetic*** (2nd ed., 2018), the chapter on software implementation, and the **FLIP** library for integer-only cores (C.-P. Jeannerod, G. Revy et al.):
  - multiplication by partial products of the significands;
  - division and square root by digit recurrence or by Newton iteration;
  - which one is cheaper depends on the cost of the core's multiplications.
- **Qfplib-M0** (M. Owen, <https://www.quinapalus.com/qfplib.html>): a hand-scheduled Cortex-M0 float library, showing that a few tens of cycles per operation are possible in assembly. It is GPL and was only read about, not read as code.
- **The C23 standard**, 7.12.7.10 (`sqrtf`) and Annex F (F.10.4.10): correctly rounded, `sqrtf(-0) = -0`, and invalid for x < 0.

## Design

A new `nofp` layer holds the integer operations (`libc/math/nofp`, selected on armv6-m and armv6s-m by `LIBC_MATH_LAYERS`, D46). `softfloat.h` holds the shared stages:
- **unpack:** the 24-bit significand and the exponent, with subnormals normalised by `stdc_leading_zeros`;
- **pack:** one rounding to nearest, ties to even:
  - from a significand carrying a guard bit and a sticky bit;
  - a subnormal result is shifted before it is rounded, so it is rounded once;
  - overflow goes to infinity;
  - a carry out of the significand reaches the exponent through the addition.

Only round-to-nearest is implemented, because it is the only direction `<fenv.h>` has without an FPU (D44), and no flags are raised.

- **`__aeabi_fmul`:**
  - Armv6-M's `MULS` keeps only the low 32 bits of a product, so the 48-bit product of the two significands comes from four 12-bit half products;
  - every partial sum stays below 2^32;
  - the product is normalised to 24 bits, the next bit is the guard, and the rest is OR-ed into the sticky bit.
- **`__aeabi_fdiv`:**
  - restoring division, one quotient bit per step, 26 steps (24 bits, the guard, one more);
  - the remainder supplies the sticky bit;
  - a Newton reciprocal would need 32 × 32 → 64 products, four `MULS` each on this core, and two iterations plus a correction step.
- **`sqrtf`** (replaces the `sp` layer's on these cores):
  - the root of the significand, two radicand bits at a time, 25 steps;
  - the remainder, below 2^28, fits in 32 bits, so there is no 64-bit arithmetic;
  - the scaling puts the exponent's parity in the radicand, which gives the result's exponent directly;
  - a square root cannot fall exactly on a tie.

### Measured and not taken

- **`__aeabi_fadd`** in the same style: exact against the hardware over 2 × 10^8 random pairs, but slower than libgcc's in the libm benchmark. Replacing libgcc's `fadd` together with `fmul` and `fdiv` made 51 cases slower. The alignment shift and the normalisation after a cancellation cost more in C on Thumb-1 than in libgcc. Not taken; libgcc's addition stays.
- **Rewriting `expf`, `logf`, `sinf` and `erff` in fixed point** is the larger gain the step named. Each needs a fixed-point polynomial with its own error analysis to keep the layer's correct rounding (D57), so they are split into steps P3.05f–h.

## Results (armv6-m, release and speed flavours, instructions per call)

| Function | Before | After |
|---|---:|---:|
| `sqrtf` | 2036 | 461 |
| `hypotf` | 8371 | 6293 |
| `acosf` | 26692 | 23828 |
| `asinhf` | 21693 | 19353 |
| `cbrtf` | 9412 | 8590 |
| `erff` | 151747 | 141024 |
| `expf` | 11458 | 11034 |
| `logf` | 12864 | 12367 |
| `sinf` | 18996 | 18350 |
| `strtof` (short), release | 1634 | 1529 |
| `strtof` (short), speed | 1008 | 903 |

The release and speed flavours give the same libm counts. In each flavour, on armv6-m and on armv6s-m, 62 cases are faster and none slower.

Size:
- `__aeabi_fmul` is 340 bytes against libgcc's 504, `__aeabi_fdiv` 296 against 512, and `sqrtf` 166 against 320;
- `math-float` −604 bytes, `math-mix` −532, `float-exact` −164.

Each helper's frame is 32 bytes (eight registers for the operands, the partial products and the exponents, on a core with eight low registers), so the stack depth of `sinf`, `expf` and `powf` grows by 24 bytes.

## Tests

- **On the host** (outside the repository):
  - `fmul` and `fdiv` (and the rejected `fadd`) are bit-identical to x86-64's SSE over 3 × 10^8 random pairs, weighted to subnormals, specials, and results at both ends of the range;
  - `sqrtf` is bit-identical to `sqrtss` over all 2^32 inputs.
- **`test/unit/softfp`** (`float_operations.c`, on the boards whose float helpers are soft calls: armv6-m and armv6s-m, plus x86-64, which compiles the nofp files into the test):
  - `fmul`, `fdiv` and `sqrtf` against the operation in double rounded to float (double has more than 2 × 24 + 2 bits, so this rounds correctly);
  - specials × specials, every float in [1, 4) with a significand ending in 0x1ff, and 40,000 random pairs;
  - among the random pairs, products of a few-bit significand with 1 + m·2^-23 or 2 − (m+1)·2^-23, so that a guard bit and a lone sticky bit at every position reach the rounding;
  - the reference goes through `volatile` doubles: GCC narrows `(float)((double)a * (double)b)` to the float operation, which on Armv6-M made the reference the helper under test;
  - mutants: dropping the multiplication's sticky bit 22 (19 failures), dropping the division's remainder (4110), and dropping the square root's remainder are all caught.
- **The float accuracy tests and libc-test** run the whole layer on every board and flavour.
