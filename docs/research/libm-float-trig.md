# Research: float-float sine, cosine and tangent for single-precision cores

Step: P2.05d1. Decision: PLAN D60. It builds on D57 (float-float and the
`sp-nearest` layer) and D48 (the double trigonometric functions).

## Sources reviewed

- M. Payne and R. Hanek, "Radian reduction for trigonometric functions",
  SIGNUM Newsletter 18(1), 1983, and K. C. Ng, "Argument reduction for huge
  arguments: good to the last bit", Sun Microsystems, 1992:
  - multiply the significand by a window of bits of 2/π, starting where
    the bits stop contributing whole periods;
  - the integer part gives the quadrant and the fraction the reduced
    argument;
  - D48 does this for double with 32-bit limbs.
- J.-M. Muller, *Elementary Functions*, 3rd ed., ch. 11: the closest
  floats to multiples of π/2 leave fractions of about 2^−30 of a unit, so
  the window must reach about 64 bits past the leading fraction bit.
- P. T. P. Tang, "Table-driven implementation of the exponential function"
  (1989), applied to sin/cos as in Tang's and Gal's accurate tables:
  - sin(a + s) = sin a cos s + cos a sin s with a = jπ/32 from a table;
  - the polynomials only see |s| ≤ π/64.
- picolibc/newlib `sinf` (SunPro, description only): float Cody–Waite
  reduction plus a large-argument path in floats. Its error is near 1 ulp.
- ARM optimized-routines `sinf`/`cosf` (description only): reduction and
  polynomial in double. That design needs double hardware.
- C23 7.12.4 and F.10.1:
  - sin(±0) = ±0 and tan(±0) = ±0;
  - sin/cos/tan of ±∞ are NaN (invalid);
  - sinpi(n) = ±0 with the sign of n, cospi(n + ½) = +0;
  - tanpi(n) = ±0 (the sign of n for an even n, of −n for an odd one);
  - tanpi(n + ½) = ±∞ (divide-by-zero; + for even n).

## Design

- **`__libc_rem_pio2f(x, &s)`** returns k mod 64 and s = d·π/32 with
  x = (k + d)·π/32 and |d| ≤ ½, for |x| ≥ 2^−6, in integers:
  1. With x = m·2^e, x·32/π = m·2^(e+4)·(2/π). The bits of 2/π before
     position e − 1 add only multiples of 64.
  2. A 128-bit window from there (from an 8-word table, 32 bytes, enough
     for the largest float) times the 24-bit m gives a 152-bit product in
     five 32-bit limbs.
  3. k is the six bits at the binary point. A fraction of ½ or more
     becomes k + 1 and a negative d (two's complement of the limbs).
  4. The leading 48 bits of |d| become a float-float, multiplied by π/32.
     The truncation is below 2^−98 of a unit, so d keeps about 48
     significant bits even when x is very close to a multiple of π/32.
  5. There is no Cody–Waite path: below 2^−6 the caller uses s = x,
     k = 0 directly.
- **`__libc_sinf_core(k, s)`** returns sin(kπ/32 + s) for any k and
  |s| ≤ π/64:
  - k + 8 = 16q + (j + 8) gives the quadrant q and j ∈ [−8, 7];
  - the table holds sin and cos of |j|π/32 as float pairs (9 rows,
    144 bytes);
  - sin s = s − s³/6 + s⁵/120 − s⁷/5040, with the cube and the 1/6 as
    float-float;
  - cos s − 1 = −s²/2 + s⁴/24 − s⁶/720, from an exact square;
  - the angle sum is formed with float-float products, so the result is
    good to about 2^−42.
  - The quadrant picks sin or cos of j·π/32 + s, and its sign.
- **The functions:**
  - `sinf` is the core at k;
  - `cosf` is the core at k + 16;
  - `tanf` is their float-float quotient;
  - `sinf`/`tanf` return x below 2^−12/2^−13, where x³/6 (x³/3) is below
    half an ulp. That also keeps the sign of −0.
- **The π-scaled forms** reduce exactly in float:
  - 32x = K + d (`roundf`, |x| < 2^23), and s = d·π/32;
  - the multiples of ½ return the exact zeros and ones above;
  - from 2^23 up every float is an integer, and from 2^24 up an even one;
  - below 2^−90, π·x is rounded once through `__libc_exp2f_scale` (D57/D58).

## Accuracy

`test/unit/float_accuracy`, x86_64:
- 2 097 152 arguments of every binade per function;
- sweeps of every float near a zero or a pole: [3.125, 3.156] around π
  for `sinf`, [1.5625, 1.578] around π/2 for `cosf`/`tanf`, around 1 for
  `sinpif` and around ½ for `cospif`/`tanpif`.

| Function | max error (ulp) | not correctly rounded |
|---|---:|---:|
| `sinf` | 0.500 | 1 of 2 228 225 |
| `cosf` | 0.500 | 0 of 2 228 225 |
| `tanf` | 0.500 | 0 of 2 228 225 |
| `sinpif` | 0.500 | 0 of 2 293 761 |
| `cospif` | 0.500 | 0 of 2 293 761 |
| `tanpif` | 0.500 | 0 of 2 293 761 |

- The Arm boards show 0 of 8192 per function.
- `test/unit/libm_trig`'s float vectors (correct rounding expected) pass
  on the layer as well.

## Test harness fixes

- The uniform samples over [−FLT_MAX, FLT_MAX] computed high − low = ∞,
  so half the arguments were NaN. They are now interpolated without that
  difference.
- NaN results no longer count as misrounded.

## Fault injection

Ten mutants (x86_64):
- caught:
  - a wrong bit in the 2/π table's fourth word;
  - the earlier j formula (j = 56 … 63 instead of −8 … −1);
  - a wrong 1/24 coefficient;
  - the reduction without k + 1 for an upper fraction;
  - `sinpif`'s ±1 sign inverted;
  - `tanpif`'s pole sign inverted;
  - `sinf` returning x up to 2^−7;
  - d without its low 24 bits.
- equivalent: a wrong last bit of the table, worth 2^−128 of a unit.
- marginal: −1/6 without its low part. That is about 2^−36 relative,
  2^−12 ulp, and only near multiples of π with |s| near π/64. It stays,
  as the float-float constant costs nothing.
