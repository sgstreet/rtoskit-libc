# Research: float-float powers and roots for single-precision cores

Step: P2.05c3. Decision: PLAN D59. It builds on D57 (the exp2 core,
float-float, the `sp-nearest` layer) and D58 (the log2 core).

## Sources reviewed

- C23 7.12.7 and F.10.4: the special cases of `pow` (F.10.4.5), `powr`
  (IEEE 754 9.2.1: 0^0, ∞^0 and 1^∞ are invalid), `pown`, `rootn`,
  `compoundn`, `rsqrt`, `cbrt`, `hypot` (an infinity wins over a NaN) and
  `sqrt`. Our double implementations encode the same cases; the float ones
  follow them and the test compares every pair of special values with them.
- P. Markstein, *IA-64 and Elementary Functions*, 2000 (ch. 8): Newton
  steps for the reciprocal square root and the cube root, with the residual
  formed exactly by a fused or exact product so that the last step lands
  within a fraction of an ulp.
- T. J. Dekker (1971) and Muller et al., *Handbook of Floating-Point
  Arithmetic*, 2nd ed., ch. 14: exact products, and division of a
  double-word by a double-word corrected by the exact remainder.
- J. Hauser, SoftFloat (<http://www.jhauser.us/arithmetic/SoftFloat.html>,
  description only): a correctly rounded square root from an integer root
  of the significand plus a sticky bit. Here the rounding itself is left to
  one integer-to-float conversion, which follows the rounding direction.
- K. Packard, picolibc float math: SunPro `powf` in float with an error
  near 1 ulp; ARM optimized-routines `powf` computes in double (D57).

## Design

- `__libc_exp2f_signed(z, negative)`: ±2^z rounded once, overflow above
  2^128.5 and zero below 2^-151.5, else the exp2 core and its scaling
  (D57). Every power ends here.
- `powf`, `powrf`: z = y · log2|x| with the log2 core (D58) and a
  float-float product; a |y| above 2^31 is out of range either way (|log2|x||
  is at least 2^-23 once x is not 1). `powf` takes the sign from an odd
  integer y (every |y| from 2^24 up is even).
- `pownf`, `compoundnf`: n is a 64-bit integer; the compiler's conversion
  computes in double on these cores, so `__libc_ff_from_i64` sums three
  exact 21-bit chunks into a float-float (about 2^-48). `compoundnf` uses
  the log2p1 core (D58) and returns 1 for |x| below 2^-90, where |n x| is
  below 2^-27.
- `rootnf`: z = log2|x| / n as a float-float quotient corrected by its exact
  remainder (`__libc_ff_divide`).
- `rsqrtf`: r = 1/sqrtf(x), then r + r (1 - x r^2)/2 with the residual from
  Dekker products; x scaled into [2^-100, 2^100] so r^2 stays normal.
- `cbrtf`: m in [1, 8), four float Newton steps from 1 + (m - 1)/7, then
  one step with the residual m - t^3 from exact products.
- `hypotf`: the squares and their sum as float-float, sqrtf corrected by the
  exact residual, operands scaled by 2^-70 or 2^90 so the squares stay
  normal, and the scaling back rounds once through `__libc_exp2f_scale`
  (subnormal results included).
- `sqrtf` (the `sp` layer, every Arm core without a VSQRT layer, that is
  armv6-m and armv6s-m): the 26- or 27-bit integer root of the significand
  shifted to an even exponent (`__libc_isqrt`), its last bit or'ed with
  "not exact", converted to float once: the conversion rounds in the
  current direction, so the result is correctly rounded in every
  direction and the function stays fenv-independent like the rest of `sp`.

## Accuracy

`test/unit/float_accuracy`, x86_64, with `sp-nearest` compiled in:

- The two-argument functions draw x from every binade. Three samples in
  four take y = z / log2 x, with z uniform in [−155, 132], so results stay
  in range; the rest take y of any binade. `powf` also gets negative x
  with an integer y.
- `hypotf` takes pairs of any magnitudes and pairs within a factor of 2.
- The integer forms take n in [−50, 50], or n = z / log2|x| (for
  `compoundnf`, z / log2(1 + x)), which reaches n near 2^62.
- Every pair of 14 special values (or special values against
  n ∈ {0, ±1, ±2, ±3, INT64_MAX, INT64_MIN}) must give exactly what the
  double function gives, the sign of zero included.

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `sqrtf` (armv6-m `sp`) | 0.500 | 0 of 16 384 (Arm) |
| `rsqrtf` | 0.500 | 2 of 20 971 521 |
| `cbrtf` | 0.500 | 1 of 29 360 129 |
| `powf` | 0.504 | 42 of 4 194 304 |
| `powrf` | 0.504 | 42 of 4 194 304 |
| `hypotf` | 0.505 | 16 of 4 194 304 |
| `pownf` | 0.500 | 25 of 4 194 304 |
| `rootnf` | 0.500 | 36 of 4 194 304 |
| `compoundnf` | 0.509 | 77 of 4 194 304 |

The Arm boards (16 384 arguments each) show 0 not correctly rounded.

## Defects found on the way

- **The double `compoundn`** (P1.14) returned 1 + n·x for |x| < 2^−60.
  That is wrong once n·x is not small (n reaches 2^63):
  `compoundn(2^−61, −2^61)` gave −0.60 instead of e^−1.
  - The float test found it, using the double function as its reference.
  - It now feeds log(1 + x) = x − x²/2 (exact to double precision below
    2^−60) to the integer-power core.
  - `test/unit/libm_exp` checks three such cases against 60-digit values.
- **`__libc_exp2f_scale`** (D57) returned +0 for a negative result that
  rounds to zero: (−1) − (−1) is +0.
  - `test/unit/libm_exp` caught it (`powf(−2, −151)` must be −0).
  - The result now takes the sign of the value.
  - `float_accuracy`'s error measure now requires the sign to match, zeros
    included. Its test of that mutant fails.

## Fault injection

Twelve mutants:
- caught on x86_64:
  - `powf`'s odd/even classification inverted;
  - the |y| shortcut taken from 2^10;
  - overflow at 2^127.5;
  - a wrong chunk shift in `__libc_ff_from_u64`;
  - `rsqrtf` without its Newton correction;
  - `cbrtf` with one float Newton step;
  - `hypotf` without the squares' low parts;
  - `compoundnf`'s "rounds to 1" shortcut from 2^−20;
  - the subnormal zero without its sign;
- caught on armv6-m: `sqrtf` without the sticky bit;
- caught in `libm_exp`: the old double `compoundn` shortcut;
- not significant: `__libc_ff_divide` without the divisor's low part. It
  is non-zero only for |n| > 2^24, where log2|x|/n is below 2^−17 and its
  effect below 2^−41. It stays for generality.
