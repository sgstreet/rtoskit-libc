# Research: libm part 4, erf, erfc, lgamma and tgamma

Step: P1.16. Decisions: PLAN D7, D45, D47, D49.

## Sources reviewed

- W. J. Cody, *Rational Chebyshev approximations for the error function*,
  Math. Comp. 23 (1969), and the fdlibm/musl `s_erf.c` (structure only).
  They use rational approximations on [0, 0.84], [0.84, 1.25], [1.25, 1/0.35]
  and [1/0.35, 28], with erfc(x) = exp(-x² - 0.5625) · R(x)/x for large x.
  This is about 1 ulp; each interval needs its own minimax fit.
- M. Abramowitz and I. Stegun, 7.1.5 and 7.1.14 (the Maclaurin series of erf
  and the continued fraction of erfc), and DLMF 7.10 (the derivatives of
  erf through Hermite polynomials).
- W. Gautschi, *Efficient computation of the complex error function*
  (SIAM J. Numer. Anal. 1970): convergence of the Laplace continued fraction
  for large arguments.
- Stirling's series and the Bernoulli numbers (DLMF 5.11.1), the Taylor
  series of log Γ(1 + z) and log Γ(2 + z) in ζ(k) (DLMF 5.7.3), and the
  reflection Γ(x)Γ(1 - x) = π/sin(πx) (DLMF 5.5.3).
- C. Lanczos (1964) and P. Godfrey's coefficients, used by Boost and glibc's
  `tgamma`: rejected. The rational form loses relative accuracy at the zeros
  of lgamma, and the error analysis is harder than for Stirling with a
  shift.
- CORE-MATH (<https://core-math.gitlabpages.inria.fr/>): correctly rounded
  `erff`, `erfcf`, `lgammaf`, `tgammaf` (float) and their hard cases.
- The standard: C23 7.12.8 and Annex F.10.5.
- libc-test `src/math/{sanity,special,ucb}` for erf, erfc, lgamma and tgamma
  (f/l). The lgamma tests read the XSI `signgam`.

## Trade-offs

| Option | For | Against |
|---|---|---|
| fdlibm rational approximations | Small, fast | About 1 ulp; each interval needs a minimax fit, which would have to be regenerated |
| Series, table and continued fraction with double-double heads | Uses only exactly derivable coefficients (Taylor, Hermite, Bernoulli, ζ); error below 2^-60 before the final rounding | More arithmetic; a 57-entry erfc table (1.8 KiB) |

Chosen: the second option. Its coefficients come from the definitions, not
from fits, which suits clean-room work and makes the accuracy argument
direct.

## Algorithms

- **erf, below ½**: the Maclaurin series (2/√π) Σ (-1)^n x^(2n+1)/(n!(2n+1)).
  x - x³/3 is computed as double-doubles, and the terms n = 2 … 13 in
  double, since the next term is below 2^-62 relative.
- **erfc, from 15/32 to 129/32**: a table of erfc(c) and (2/√π)e^(-c²) for
  c = ½ + k/16 (57 entries).
  - erfc(c + h) = erfc(c) - (2/√π)e^(-c²) · S, with
    S = Σ (-1)^(m-1) H_(m-1)(c) h^m/m!, which is the Taylor series of
    ∫ e^(-t²).
  - The Hermite polynomials come from their recurrence
    H_(n+1) = 2cH_n - 2nH_(n-1).
  - With |h| ≤ 1/32 the terms fall below 2^-60 of S by m = 15.
  - The cancellation erfc(c) - … loses at most 2 bits at c = 4.
- **erfc, from 4 to 27.3**: erfc(x) = e^(-x²)/(√π K(x)), where K is the
  Laplace continued fraction x + ½/(x + 1/(x + (3/2)/(x + …))).
  - K is evaluated backwards from 30 levels as double-doubles. Python
    experiments show below 2^-68 relative from x = 4; 2 needs 80 levels,
    which is why the table covers [½, 4).
  - x² is an exact product. e^(-x²) comes from the D47 exp core, and the
    result is scaled once, so subnormal results round once.
  - Beyond 27.3 the result underflows.
- **The other cases**:
  - erf(x) = 1 - erfc(x) above ½;
  - erfc(x) = 1 - erf(x) below ½;
  - erfc(-x) = 2 - erfc(x);
  - erf saturates to ±(1 - tiny) from 6, and erfc to 2 - tiny below -6;
  - for |x| < 2^-28, erf(x) = (2/√π)x rounded once, through the 2^200
    scaling (subnormal results included).
- **lgamma of positive x**:
  - From 12 up, Stirling's series:
    (y - ½) log y - y + ½ log 2π + Σ B_2k/(2k(2k - 1)y^(2k-1)). The
    product and the logarithm are double-doubles; ten terms are below 2^-66
    of the result at 12.
  - Below 12, it shifts up: log Γ(x) = log Γ(x + n) - log(x(x+1)…(x+n-1)).
    x + k and the product are double-doubles, so no rounding of x + n enters.
  - Within ¼ of 1 and 2, it uses the ζ series of log Γ(1 + z) and
    log Γ(2 + z), to k = 40. This keeps the relative accuracy at the zeros 1
    and 2. ζ(k) - 1 is tabulated, the first two terms are double-doubles.
  - Beyond 2^60, x(log x - 1) is computed as a scaled double-double with one
    rounding. Stirling's other terms are below 2^-60 relative there.
- **lgamma of negative x**: the reflection log π - log|sin πx| -
  log Γ(1 - x).
  - 1 - x is exact as a double-double.
  - sin πx comes from the exact reduction x = n + f, |f| ≤ ½, through the
    D48 kernels, which also give the sign of Γ.
- **tgamma**:
  - The factorials up to 22! are exact products (23! needs 56 bits).
  - |x| < 2^-54 gives 1/x.
  - Otherwise tgamma is exp(lgamma) with the reflection's sign, through the
    exp core and one scaled rounding. Overflow comes past 171.624 and
    underflow below about -184, with the sign of Γ.
- **Special cases (F.10.5)**:
  - erf(±0) = ±0; erf(±∞) = ±1; erfc(-∞) = 2; erfc(+∞) = +0.
  - lgamma(1) = lgamma(2) = +0. Poles give +∞ with divide-by-zero.
    lgamma(±∞) = +∞.
  - tgamma(±0) = ±∞ with divide-by-zero. A negative integer or -∞ is a
    domain error.
- **Float functions** return `(float)double_function(x)`; **long double**
  ones call the double ones (D7).

## signgam

libc-test's lgamma tests read `signgam` (XSI). C does not reserve the name,
so the library cannot define it. The test shim provides it:
`test/libc-test/shim/include/signgam.h` is forced into the math suite with
`-include`, and wrappers record the sign of Γ(x) beside the library's
unchanged lgamma.

## Tests and vectors

- libc-test: `math/erf`, `math/erfc`, `math/lgamma`, `math/tgamma` (f/l,
  12 tests) pass on all six boards.
- `test/unit/libm_special/vectors.inc` holds 2400 vectors, 300 for each of
  erf, erfc, lgamma and tgamma, in double and float, generated from mpmath
  at 600 bits. They include the regions near the zeros of lgamma at 1 and 2,
  negative arguments, and x up to 2^1000.
- The worst error to nearest is 0.50 ulp, in double and float.
- Also checked: the F.10.5 special cases, the exact factorials 0! … 22!,
  and the overflow and underflow boundaries (erfc(27), erfc(30), tgamma
  past 171.6 and below -190, and the sign of Γ there).
