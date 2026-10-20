# Research: sharing the float kernels and sizing their tables

Step: P2.05e4. Decision: PLAN D68. It reviews the float-float layer of
D57–D67, now that every float function on the single-precision cores is in
it.

## Sharing between the `f` and `d` variants

The step text predates D57. Sharing a kernel between `float` and `double`
means one of two things:
- **Evaluating the float function in double**, which is what armv7e-m-dp
  and x86_64 do: their float functions are the double wrappers, and the
  kernels are shared by construction.
- **A common kernel in some third precision.** On the soft-double cores
  that is either double (soft, the cost D57 measured and removed) or
  float-float (too short for double).

So on the single-precision cores the f and d kernels stay apart by
design (D57). A program that uses both `sinf` and `sin` links both cores,
and one that uses only float links no double arithmetic at all, which is
the size goal of §1. Nothing further is shared across precisions.

## Sharing inside the float layer

| Change | Effect |
|---|---|
| `asinhf` calls `__libc_asinhf_core` (D67) instead of its own copy of the same regions | One copy of the asinh regions; a program with `asinhf` and `casinhf` saves the duplicate. A lone `asinhf` grows by the call (see `docs/size`). |
| `clogf`'s far-from-1 path is `__libc_clogf_modulus` (D67) | One copy of the scaled log-modulus for `clogf` and the inverse functions. |
| `sinhf`/`coshf`/`tanhf` keep their own code | `__libc_sinhcoshf_core` (D66) computes both functions and a square root; a lone `sinhf` would link more than it has now. |
| The exp2, log2, atan2, sin and expf-pair cores | Already shared by every function of their family (D57–D62). |

## Table sizes: measured

`test/unit/float_accuracy`, x86_64 release (2 097 152 arguments per
function plus the sweeps), before → after:

- **exp2 core: 32 → 8 entries of 2^(j/8)** (256 → 64 bytes).
  - Halving the table alone failed: `erff` misrounded 262 in 2.36M (limit
    236) and the m1 forms reached 0.516 ulp. A degree-6 term did not help.
  - The error was not the truncation but the r²/2 term rounded in float:
    2^−24 of r²/2 is 2^−36 at |r| = 0.022.
  - With r² as an exact Dekker product, as D58 did for log1p, the series is
    good to 2^−44 up to |r| = 0.044. Every exponential got better at the
    same time:
    - `expf` 40 → 5 misrounded of 4.3M;
    - `expm1f` 0.506 → 0.500 ulp, `exp2m1f` 0.508 → 0.500, `exp10m1f`
      0.510 → 0.500;
    - `erff` 25 → 7;
    - `sinhf` 235 → 49.
  - 16 entries measured slightly better than 8 (`expf` 3, `erff` 2), but
    both are far ahead of the old 32-entry core, so 8 was taken.
- **atan2 core: 17 → 9 entries of atan(k/8)** (136 → 72 bytes).
  - |u| ≤ 1/16, and the series to u^9 leaves 2^−43.5. All inverse
    trigonometric functions stay at 0.500 ulp, with 0 or 1 misrounded per
    2.4M.
  - 5 entries (|u| ≤ 1/8) would need two more terms and leave a float tail
    of 2^−38, so they were not tried.
- **log2 core: 23 entries kept.**
  - 13 entries of 1/(1 + j/16) with the log1p series extended to f^10 were
    measured:
    - `compoundnf` 0.526 ulp (fails the 0.52 bound);
    - `lgammaf` 0.506 ulp and 223 misrounded;
    - `powf` 0.509 ulp.
  - The pow-like functions multiply the logarithm's absolute error, and the
    f³/3 term in float costs 2^−39 at |f| = 0.046. Recovering it needs that
    term in float-float, about 60 bytes of code for 120 bytes of table.
    Reverted.
- **sin core: 9 rows of jπ/32 kept** (144 bytes).
  - π/16 would save 64 bytes, but it changes `__libc_rem_pio2f` (k mod 32
    and the window position) and needs one more term in each polynomial.
    The reduction is the riskiest code in the layer, so it was not changed
    for 64 bytes.
- **lgamma zero expansions: to c6** (728 → 672 bytes). The c7 term is below
  2^−43.5 of the result; every float of (−9, −3) re-swept stays at
  0.500 ulp with the same misrounds.
- **Stirling's 1/(1188x^9) term kept** (8 bytes; without it 2^−40 at 8).

The tables shrink by 192 + 64 + 56 = 312 bytes, and the accuracy improved
everywhere it changed.

## Test resolution

Four mutants of this step survived:
- the r^6/720 term dropped, worth 2^−36.5;
- the atan reduction with floor instead of round (|u| up to 1/8: 2^−36.5);
- c6 of the lgamma zero expansions dropped;
- the u^11 term of atan, which is now removed.

Errors near 2^−36 relative are 2^−12 ulp. They change a correctly rounded
result about once in 4000 near-halfway cases, which the 1/10 000 bound
cannot resolve against the reference's own double rounding. The design
budget stays 2^−40 (D57): the r^6/720 term and c6 are kept for it, and the
u^11 term, 2^−43.5, is not needed.
