# Research: fixed-point exponentials and logarithms on Armv6-M

Step: P3.05f. Decision: PLAN D86.

## Starting point (P3.05e speed baseline, armv6-m, instructions per call)

| Function | Instructions |
|---|---:|
| `exp2f` | 9269 |
| `expf` | 11034 |
| `log2f` | 10547 |
| `logf` | 12367 |

The `sp-nearest` layer (D57, D58) computes in float-float to about 2^-40 and rounds once. On a core without an FPU, each float operation is a soft-float call. An exact product takes about ten of them, so `expf` costs about 70 calls of 100–150 instructions each.

## Sources reviewed

- **CORE-MATH** (P. Zimmermann et al., <https://core-math.gitlabpages.inria.fr/>), the `binary32` exponentials and logarithms:
  - a fast path in double precision whose error bound is known;
  - a rounding test that accepts the result when the error interval holds no rounding boundary;
  - otherwise an accurate path (Ziv's strategy).
- **A. Ziv, *Fast evaluation of elementary mathematical functions with correctly rounded last bit*** (ACM TOMS 17, 1991): the rounding test and the retry with more precision.
- **J.-M. Muller, *Elementary Functions: Algorithms and Implementation*** (3rd ed., 2016), ch. 6 (table-based range reduction, Tang's method) and ch. 11 (the table maker's dilemma).
- **P. T. P. Tang, *Table-driven implementation of the exponential function in IEEE floating-point arithmetic*** (ACM TOMS 15, 1989), and the logarithm counterpart (1990): 2^(j/32) tables, and log tables at 1 + j/N with a short polynomial.
- **ARM optimized-routines** `expf`/`exp2f`/`logf`/`log2f` (<https://github.com/ARM-software/optimized-routines>): table-driven in double precision for cores with a double FPU, which Armv6-M lacks.
- **Qfplib-M0** (M. Owen, <https://www.quinapalus.com/qfplib.html>): fixed-point `exp` and `ln` for Cortex-M0 by shift-and-add (CORDIC-like) iterations. They are fast, but not correctly rounded. It is GPL and was only read about, not read as code.
- **FLIP** (C.-P. Jeannerod, G. Revy et al.): polynomial evaluation in 32-bit integers for integer-only cores, written to schedule well on VLIW machines.

## Design

The functions sit in the `nofp` layer's `speed/` directory (D77), so only the speed flavour on armv6-m and armv6s-m uses them:
- `libc/libc.mk` archives `libc/math/<layer>/speed` in the speed flavour;
- `sp-nearest/subdir.mk` then drops its own files of the same name.

The size flavour is byte-identical.

Each function keeps the float-float code as its fallback (Ziv):
- **fast path:** computes 2^n (1 + F·2^-32) in 32-bit fixed point;
- **rounding test:** F's 9 bits below the float's last bit are compared with the halfway point. If they differ from it by more than the error bound, rounding F rounds the exact value the same way, so the result is returned;
- **otherwise:** the float-float path runs, giving exactly the result the size flavour gives.

Arguments outside the fast range also take the float-float path: subnormal or overflowing results, NaN, infinities, zero and negative logarithm arguments.

### Exponentials (`__libc_exp2f_fixed`)

- **Reduction:** z = x (`exp2f`) or x / ln 2 (`expf`), as a signed 64-bit integer in units of 2^-37.
  - `expf` builds it from the 24-bit significand times 1/ln 2 to 64 bits, with three 32 × 32 products.
  - n = floor(z) is the result's exponent, j the next 5 bits, and r ∈ [0, 2^-5) the low 32 bits.
  - Every value is unsigned, so the core needs no signed multiplication.
- **2^(j/32) − 1:** a 32-entry table, in units of 2^-32.
- **2^r − 1:** the Taylor polynomial of degree 4 in r·ln 2; the next term is below 2^-34. It is evaluated by Horner's rule, with each coefficient scaled to fill 32 bits, and the products' high halves shifted by 7 between steps.
- **Combination:** (1 + t)(1 + p) − 1 = t + p + t·p. It is saturated below 2: the true value is below 2, and the rounding goes up as it must.
- **Fast range:** |x| < 126 for `exp2f` and |x| < 87 for `expf`, where every result is normal.

### Logarithms (`__libc_log2f_fixed`)

- **Reduction:**
  - x = 2^e·m, with m ∈ [√½, √2) as an unsigned Q31 number;
  - c' is the reciprocal of a Q31 number near 1/c, for c = 1 + j/32 (j = −9 … 13), the same reduction as D58;
  - r = m / c' − 1 is exact: the Q62 product of m and the table's 1/c, less 1.
- **log2(1 + r):** r·Q(r), with Q the series (1 − r/2 + r²/3 − …) / ln 2 to the r⁵ term. It is evaluated by Horner's rule in signed Q31, with r in Q36 and every step rounded to nearest. The final step and the constant 1/ln 2 are unsigned.
- **Relative accuracy:**
  - r·Q uses r normalised to 32 bits;
  - near 1 (e = 0, j = 0) the result is that product alone, so it keeps r's relative accuracy however small r is;
  - elsewhere |log2 x| ≥ 0.022, and e + log2 c' + r·Q is summed in units of 2^-55.
- **`logf`:** multiplies the normalised result by ln 2 to 64 bits.

### Arithmetic

- **`__libc_fixed_wide`:** Armv6-M's `MULS` keeps 32 bits, so the 64-bit product comes from four 16-bit half products. It is one shared out-of-line function: `-Os` already kept a local copy of it per object, and four copies broke the speed budget.
- **Signed high half:** from the unsigned one, corrected for each negative operand.
- **libgcc:** the 64-bit variable shifts and the leading-zero counts call `__aeabi_llsl`/`__aeabi_llsr` and `__clzsi2`/`__clzdi2`.

## Error bounds, verified over every argument

The core code compiles unchanged on the host. A harness ran every float argument in each fast range through it and compared F with `exp2l`/`expl`/`log2l`/`logl` on x86-64 (64-bit significand, error far below 2^-32 of F):

| Function | Arguments | Largest error of F (units of 2^-32) | Bound used | Falls back | Not correctly rounded on the fast path |
|---|---:|---:|---:|---:|---:|
| `exp2f` | 2,247,622,656 | 2.68 | 6 | 0.62 % | 0 |
| `expf` | 2,237,399,040 | 2.71 | 6 | 0.60 % | 0 |
| `log2f` | 2,139,095,039 | 4.49 | 8 | 3.32 % | 0 |
| `logf` | 2,139,095,039 | 4.23 | 8 | 3.32 % | 0 |

Every argument is covered, so these bounds are proven for this code, not estimated. Each bound used is above the measured largest error. Where F is spread evenly, the fallback rate is (2·bound + 1) / 512, as for the logarithms. The exponentials fall back less often: about three quarters of their arguments are below 2^-24 in magnitude, where F stays clear of the halfway point.

The logarithms' error is largest just below 1 and at the bucket edges near |log2 x| = 0.022:
- the term r·Q and the table entry cancel there;
- before every Horner step was rounded, the largest error was 5.45 units.

## Measured and not taken

- **Inline helpers:** with `__libc_fixed_wide` inline (four local copies) and the table as an array of `{uint32, int64}` structures (padded to 16 bytes each), `math-float` was 10728 bytes against a speed budget of 10604. One shared helper and two arrays give 10520 bytes, for about 40 instructions more per `expf` (336 → 375).
- **The size flavour:** gains nothing from this step. The fast paths cost about 1.5 KB of code, which the size flavour's +0 % budget does not have.

## Results (armv6-m speed flavour, instructions per call; armv6s-m the same)

| Function | Before | After |
|---|---:|---:|
| `exp2f` | 9269 | 316 |
| `expf` | 11034 | 375 |
| `log2f` | 10547 | 583 |
| `logf` | 12367 | 2210 |

`logf`'s figure is the mean over the benchmark's 16 arguments, about two of which fall back (12,000 instructions each); its fast path costs about 650. Code: `libc.a` +3.5 KB in the speed flavour; `math-float` 9036 → 10520 (limit 10604), `math-mix` within its limit.

## Tests

- **`test/unit/libm_dense` (new, every board and flavour):**
  - every 127th float in [0.906, 1.094) and every 31st in [1.39, 1.44) for `logf` and `log2f`;
  - every 1021st in ±[16, 32) and every 2^17th across ±[2^-30, 87) or ±[2^-30, 126) for `expf` and `exp2f`;
  - each against the double function rounded to float, allowing one in ten thousand plus one, as the float accuracy test does.
  - The three results not correctly rounded on armv6-m come from the float-float path and are the same in the size flavour.
  - A mutant with both bounds set to 0, so that it never falls back, fails on all four functions; `test/unit/float_accuracy` alone missed it for the logarithms.
- **`test/unit/float_accuracy` and libc-test** run every board and flavour.

## P3.05g: sine, cosine and tangent

Decision: PLAN D87.

### Starting point (speed flavour, armv6-m, instructions per call)

| Function | Instructions |
|---|---:|
| `sinf` | 18350 |
| `cosf` | 18281 |
| `tanf` | 36223 |

### Design

- **Reduction:** the existing integer Payne–Hanek reduction (D60) is kept, with its float-float output s, |s| ≤ π/64. Its float-float work, two `scalbnf` calls and a product by π/32, is now most of the remaining cost.
- **`__libc_sinf_fixed(k, s)`:**
  - s becomes a signed integer in units of 2^-48, exact for |s| ≥ 2^-24;
  - u = s² in units of 2^-38;
  - cos s − 1 = u(−1/2 + u/24 − u²/720) and sin s = s(1 + u(−1/6 + u/120)), each by Horner's rule in Q31 (the next terms are below 2^-40);
  - the angle sum uses a 9-entry table of sin and cos of jπ/32 in units of 2^-62, and the same quadrant logic as the float-float core: first·(1 + cm) + second·sin s, summed in units of 2^-62.
  - second·s uses four 32-bit partial products, so the table's and s's low bits count.
  - Sine quadrants at j = 0, where the result is s itself, decline when |s| < 2^-14, because 2^-48 units no longer give 32 bits of relative accuracy there.
- **`tanf`:** the sine over the cosine of the same core:
  - divide by the cosine's top 32 bits (Armv6-M's `__aeabi_uldivmod`);
  - take one more quotient bit from the remainder when the sine is the smaller;
  - subtract the quotient times the cosine's low 32 bits over the cosine, at most 3 by a short loop.
  - A first version divided by the cosine's top 31 bits alone and measured 8.16 units of error; this one measures 1.39.

### Error bounds, verified over every argument

As for P3.05f, the host ran the core code, together with the real `__libc_rem_pio2f` (x86-64 with `-mlong-double-64`, which the library's headers require), over every float from 2^-12 up (2^-13 for `tanf`), against the double functions:

| Function | Arguments | Largest error (units of 2^-32) | Bound used | Falls back | Not correctly rounded on the fast path |
|---|---:|---:|---:|---:|---:|
| `sinf` | 1,174,405,120 | 1.28 | 3 | 1.38 % | 0 |
| `cosf` | 1,174,405,120 | 1.29 | 3 | 1.37 % | 0 |
| `tanf` | 1,182,793,728 | 1.39 | 4 | 1.75 % | 0 |

For `tanf`, the first version's divisor of 31 bits had a largest error of 8.16 units, above its bound of 8. Its fallback rate was 3.3 %, and no result was wrong, but only by luck; the exhaustive run found it.

### Size: the speed budget

- `math-float` links `sinf` and `cosf`. With the fast paths it is 11460 bytes, against 10604 under the speed flavour's +10 % (Phase 2 + 10 %).
- The cost is the core (782 bytes, with its 144-byte table) and 76 bytes each in `sinf` and `cosf`.
- The user chose to raise the budget for the speed flavour's math images, `math-float` and `math-mix`, to +20 % (`docs/size/budget.json`, `program_margins_percent`; D87), rather than leave the trigonometric functions out.
- `math-float` is at +18.9 %. The other images keep +10 %, and the size flavour +0 %.

### Results (armv6-m speed flavour; armv6s-m the same)

| Function | Before | After |
|---|---:|---:|
| `sinf` | 18350 | 4729 |
| `cosf` | 18281 | 3742 |
| `tanf` | 36223 | 5140 |

### Tests

- **`test/unit/libm_dense`:**
  - sweeps `sinf`, `cosf` and `tanf` over every 2039th float in ±[2^-12, 8) and every 2^17th float from 8 to 2^127;
  - 46 arguments that the fast path rounds the wrong way without its error test, found on the host over every binade, must be correctly rounded on every board (expected values from mpmath at 200 bits).
  - A mutant with every bound at 0 misses all of them. The sweeps alone tolerated it (7 of 123424 wrong, within the 1/10000 allowance).
  - Two `tanf` arguments the float-float quotient misses on every float-float board (D57's allowance) are left out.
