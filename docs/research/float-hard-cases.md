# Research: correctly rounded float functions through double

Step: Q19 (found in P4.05/P4.06). Decision: PLAN D98.

## The defect

On x86_64 and armv7e-m-dp, every float function in the generic layer is
its double function's result rounded to float, `(float)f(x)` (D45, D47).
D47 claims these results are correctly rounded to nearest.

- **What P4.05 found:** an exhaustive comparison with Arm Optimized Routines
  found 8 arguments that are rounded wrongly, by 1 ulp: `exp2f` ×2, `logf` ×3,
  `log10f` ×1 and `cosf` ×2.
- **How close they are to a halfway point:** the exact value lies 2^-30 to
  2^-34 float ulps from the point halfway between two floats. That is 2^-54
  to 2^-58 relative to the result (measured with mpmath at 300 bits).
- **Why the double result cannot decide it:** the double result carries
  29 more bits than float, and up to one double ulp of error (D47). At that
  distance it can land on the halfway point, or on its wrong side, and the
  second rounding, to float, then goes the wrong way. This is the classic
  double-rounding hazard.
- **Why it also happens with a correctly rounded double:** it would happen
  even if the double result were correctly rounded, whenever the exact value
  lies within half a double ulp of a float halfway point.

The float-float layer on the soft-float cores (`sp-nearest`, D57) misses the
same 8 arguments, and more:

- an exhaustive host run of the layer finds 818 misrounded `exp2f` arguments
  in 2^32;
- its float-float precision is about 2^-48, so it can resolve fewer hard
  cases still;
- D57 documents this layer as below 1 ulp and not always correctly rounded,
  so those results are within its stated contract.

D47's contract is the one that is broken, and only on the double path.

## Sources reviewed

- **Ziv's strategy** (A. Ziv, *Fast evaluation of elementary mathematical
  functions with correctly rounded last bit*, ACM TOMS 17, 1991): evaluate
  with an error bound, and when rounding the approximation cannot be shown
  correct, evaluate again more precisely. It is correct for every argument,
  but it needs a more precise second evaluation for every function.
- **The Table Maker's Dilemma and hardest-to-round cases** (V. Lefèvre and
  J.-M. Muller, *Worst cases for correct rounding of the elementary functions
  in double precision*, ARITH 15, 2001; J.-M. Muller et al., *Handbook of
  Floating-Point Arithmetic*, 2nd ed., ch. 12): for a finite format, the
  hardest cases can be found by exhaustive search. Once they are known, a
  precision that separates every other argument from a halfway point is
  enough.
- **CORE-MATH** (<https://core-math.gitlabpages.inria.fr/>; P. Zimmermann et
  al., *The CORE-MATH Project*, ARITH 2022): correctly rounded binary32
  functions evaluate in double with an error bound. Arguments that an
  exhaustive search shows cannot be decided at that precision are handled as
  explicit exceptions, an argument with its correctly rounded result. This
  note takes the idea, not any code or table: our exceptions come from our
  own search of our own functions.
- **Round to odd** (S. Boldo and G. Melquiond, *When double rounding is
  odd*, 2005, and *Emulation of FMA and correctly rounded sums: proved
  algorithms using rounding to odd*, IEEE TC 57, 2008): double rounding is
  innocuous when the first rounding is to odd and has at least two more bits.
  It needs the double result to be the exact value rounded to odd, which our
  double functions do not produce, so it does not apply.
- **Double rounding and the directed modes:** with the first rounding to
  double and the second to float in the same direction, the result equals one
  direct rounding. Only round to nearest has the hazard (Handbook,
  "double rounding").

## Chosen approach

1. **An exhaustive search of every unary float function on the double
   path**, on the host, against MPFR:
   - every one of the 2^32 arguments is evaluated through our own x86_64
     functions;
   - MPFR (correctly rounded to float, subnormals included) decides only
     the arguments whose double result lies within 2^-22 float ulps of a
     float halfway point, about 2^7 double ulps, far above the double
     functions' one-ulp error, or whose float result differs from the
     double result rounded;
   - every other argument is correctly rounded, because its double result
     cannot be on the wrong side of a halfway point.

   The searched functions are the 36 transcendental ones: acos, acosh,
   acospi, asin, asinh, asinpi, atan, atanh, atanpi, cbrt, cos, cosh, cospi,
   erfc, erf, exp10, exp10m1, exp2, exp2m1, exp, expm1, lgamma, log10,
   log10p1, log1p, log2, log2p1, log, rsqrt, sin, sinh, sinpi, tan, tanh,
   tanpi and tgamma, each in float. The exact functions (floor, fmod and the
   like) need no search. The binary functions (`powf`, `atan2f`, `hypotf`,
   `powrf`, `pownf`, `rootnf`, `compoundnf`) cannot be searched over 2^64
   argument pairs and stay sampled (P4.05).
2. **Each hard case found is an exception in its function:** the argument
   and its correctly rounded result (8 bytes).
   - The side of the result on which the exact value lies need not be
     stored. In every case found, the double result is exactly the float
     halfway point; the window allows a few double ulps from it. The exact
     value lies between that point and the result, so the double result
     gives the side.
   - The exceptions are looked up only when the double result is near a
     float halfway point (`__libc_float_round`, `internal/math.h`). That
     test is two integer operations on the double's low 29 bits, so an
     ordinary argument pays a few instructions.
   - The lookup itself is out of line and shared
     (`libc/math/__libc_float_case.c`, 88 bytes on Thumb-2). The first
     version inlined it into every function and pushed `math-float` on
     armv7e-m-dp over the size budget.
   - In a directed mode the result is the stored value nudged by
     2^-30 of itself toward the exact value. The hardware addition then
     rounds it in the current direction and raises inexact. This is correct
     because the exact value lies strictly between the stored result and
     its neighbour.
3. **Coverage:** the search is the test, and it is kept as
   `tools/scripts/float-hard-cases`. The arguments found are also unit-test
   vectors on the double-path boards (`test/unit/float_hard`, x86_64 and
   armv7e-m-dp), checked in every rounding mode.

The soft-float layer is not changed; its misrounded arguments are an open
question (Q26).

## Test vectors

The hard cases found by the search, with their correctly rounded results:
the `cases` tables in the changed function files and in
`test/unit/float_hard/float_hard.c`.
