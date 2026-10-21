# Phase 4: code size against Arm Optimized Routines

Plan: `docs/plan/phase-4.md` (P4.04). Decisions: PLAN D91–D93, D95.
Speed: `docs/perf/phase-4.md`. AOR revision `f67e7daa`.

## Method

`make aor`, then `tools/scripts/size-report aor --record`. The results are
kept in `docs/size/aor-<arch>.json`; `size-report aor --from-record` prints
the tables below.

- **Per routine:** what one routine adds to an empty program, with its tables
  and the helpers it pulls in, but not the runtime the empty program already
  has.
  - `tools/aor/size-main.c` is linked once alone and once with the routine
    forced in (`-u`); the difference is in code (text, rodata, unwind) and RAM
    (data, bss, TLS).
  - This is done for our size flavour, our speed flavour (each with libgcc),
    and AOR, with `libaor-math-size.a` (`-Os`) on our size flavour.
  - The routines listed are the AOR entry points that a board's own reference
    or benchmark programs contain, so a soft-float helper that the FPU makes
    unnecessary is not listed.
- **Images:** the `test/size` reference programs of the release (size) build,
  relinked through `aor-build`'s `link()` with AOR's string, math (`-Os`) and
  fp archives. The map check confirms every substitution. The AOR image is set
  beside both of our flavours.
- **Stack depths** are not compared. AOR's assembly carries no call-graph
  information (`-fcallgraph-info`), so `size-report`'s depth analysis cannot
  follow it, and a depth from the C parts alone would understate it.

## The EHABI unwinder

AOR's `strcmp-armv6m.S` (Armv6-M) and `strlen-armv6t2.S` (Thumb-2) keep
unwind directives. Their `.ARM.exidx` entries refer to
`__aeabi_unwind_cpp_pr0`, so linking either pulls libgcc's EHABI unwinder
(`unwind-arm.o`, `libunwind.o`, `pr-support.o`): about 3.6 KB of code.

- The routines themselves are small: 136 bytes for `strcmp`, 216 bytes for
  `strlen`. The rest of the 3970 and 4122 bytes in the tables is the
  unwinder.
- This is the cost of linking those objects as AOR builds them. A user could
  avoid it by discarding the unwind tables. Our library's assembly and C carry
  no unwind references (`-fno-unwind-tables` on x86_64; on Arm, the default for
  C and no directives in our `.S`), so they never pay it.
- The `hello-puts` and `malloc-heavy` rows on the Thumb-2 cores (+4 KB) are
  this effect alone.

## Summary: reference images

Code bytes the AOR image adds to our size flavour's image (negative: smaller).

| Program | x86_64 | armv6-m | armv6s-m | armv7e-m | armv7e-m-dp | armv8-m.main |
|---|---:|---:|---:|---:|---:|---:|
| hello-puts | 0 | 0 | 0 | +4032 (+214 %) | +4032 (+214 %) | +4036 (+216 %) |
| hello-printf-int | 0 | 0 | 0 | +196 (+3 %) | +196 (+3 %) | +208 (+3 %) |
| hello-printf-float | 0 | 0 | 0 | -164 (-2 %) | +200 (+3 %) | -176 (-2 %) |
| malloc-heavy | 0 | 0 | 0 | +3966 (+207 %) | +3966 (+207 %) | +3970 (+209 %) |
| math-mix | +10101 (+61 %) | +6304 (+19 %) | +6304 (+19 %) | +8944 (+33 %) | +9352 (+80 %) | +9008 (+34 %) |
| float-exact | 0 | +916 (+24 %) | +916 (+24 %) | 0 | 0 | 0 |
| math-float | -8331 (-64 %) | +1892 (+21 %) | +1892 (+21 %) | +3084 (+64 %) | -4972 (-55 %) | +3142 (+66 %) |

RAM does not change in any image.

## What the numbers say

**String routines.** Ours are the smallest everywhere: on Thumb-2, `memchr`
is 88 bytes against 248, and `memset` is 48 against 88 (108 in our speed
flavour). AOR's `strlen` and `strcmp` are 216 and 136 bytes against our 44
and 20, before the unwinder. On the Thumb-2 cores this, and nothing else, is
where `hello-printf-int` gains about 200 bytes.

**Soft-float, Armv6-M.**

- AOR's comparisons are a quarter to a third of libgcc's:
  - `fcmplt` 136 against 402 bytes;
  - `dcmplt` 180 against 642;
  - the flag-returning `cfcmple`/`cdcmple` 88–128 against 402–642.
- `fadd`/`fsub` are 724 against libgcc's 810–826.
- AOR's `fmul` (554) is larger than ours (402). Ours is the soft `<fenv.h>`
  one, which AOR's is not.
- `float-exact` grows 916 bytes with AOR, because its program pulls in AOR's
  add and the whole set of comparisons where libgcc shared members.

**Soft-float, single-precision FPU (armv7e-m, armv8-m.main).**

- AOR's conversions are a fraction of libgcc's:
  - `d2lz` 140 against 1926 bytes, and `f2lz` 124 against 1654: libgcc's
    64-bit conversions pull in soft double arithmetic;
  - `i2d` 48 against 890, and `f2d` 104 against 890.
- AOR's double arithmetic is larger: `dadd` +418, `dmul` +214, `ddiv` +102
  bytes.
- On armv7e-m-dp only the 64-bit conversions are calls, and there libgcc's
  are slightly smaller (`d2lz` 110 against 140).

**libm, float functions.**

- **On armv7e-m-dp and x86_64,** AOR's double-evaluated float functions are
  far smaller than ours.
  - armv7e-m-dp: `expf` 592 against 2308 bytes, `powf` 1452 against 5534,
    `sinf` 880 against 2476.
  - x86_64: `expf` 662 against 2990, `powf` 1703 against 8432.
  - Ours share our double cores (D47), so each float function carries their
    double-double tables.
  - `math-float` is 55–64 % smaller with AOR.
- **On the cores with a soft or single-precision FPU,** AOR's float functions
  bring soft double arithmetic and are 1.3–2.2 KB larger than ours: `expf`
  2864 against 948 bytes on armv7e-m. `math-float` grows 21–66 %.

**libm, double functions.**

- **Close:** `exp`, `exp2`, `log2` and `pow` are within −1.5 to +2.2 KB of
  ours either way.
- **AOR larger:** AOR's `log` is 0.5 KB larger on armv7e-m-dp and 1.5–2.4 KB
  larger elsewhere (its 128-entry table).
- **AOR smaller:** AOR's `erf` is 1.7–3.5 KB smaller on armv7e-m-dp, x86_64
  and armv6-m, and level on the single-precision cores.
- **math-mix** is a mix of float and double functions, and so pays AOR's
  double tables as well as the float functions' soft double. It grows on every
  board: 19 % on Armv6-M to 80 % on armv7e-m-dp.

## Per-board tables

`tools/scripts/size-report aor --from-record`.

### x86_64

Reference programs, release build: code (text + rodata + unwind) and RAM (data + bss + TLS) in bytes; AOR is the size flavour with AOR's string, math (-Os) and fp archives linked in.

| Program | code size | code speed | code AOR | AOR − size | RAM size | RAM AOR | AOR routines linked |
|---|---:|---:|---:|---:|---:|---:|---|
| hello-puts | 2300 | 2412 | 2300 | 0 | 432 | 432 | — |
| hello-printf-int | 10352 | 10578 | 10352 | 0 | 432 | 432 | — |
| hello-printf-float | 10399 | 10625 | 10399 | 0 | 432 | 432 | — |
| malloc-heavy | 2517 | 2675 | 2517 | 0 | 96 | 96 | — |
| math-mix | 16623 | 16623 | 26724 | +10101 | 40 | 40 | `cosf`, `exp`, `expf`, `log`, `logf`, `pow`, `powf`, `sinf` |
| float-exact | 3256 | 3256 | 3256 | 0 | 40 | 40 | — |
| math-float | 13045 | 13045 | 4714 | -8331 | 40 | 40 | `cosf`, `expf`, `logf`, `powf`, `sinf` |

What one routine adds to an empty program (code bytes, its tables and helpers included; RAM where any).

| Routine | size flavour | speed flavour | AOR | AOR − size |
|---|---:|---:|---:|---:|
| `cosf` | 3193 | 3193 | 833 | -2360 |
| `erf` | 6877 | 6877 | 5180 | -1697 |
| `erff` | 6897 | 6897 | 2825 | -4072 |
| `exp` | 2970 | 2970 | 2813 | -157 |
| `exp2` | 3239 | 3239 | 2824 | -415 |
| `exp2f` | 3259 | 3259 | 653 | -2606 |
| `expf` | 2990 | 2990 | 662 | -2328 |
| `log` | 2660 | 2660 | 4992 | +2332 |
| `log10f` | 2838 | 2838 | 628 | -2210 |
| `log2` | 3221 | 3221 | 2992 | -229 |
| `log2f` | 3241 | 3241 | 612 | -2629 |
| `logf` | 2680 | 2680 | 620 | -2060 |
| `pow` | 8408 | 8408 | 8252 | -156 |
| `powf` | 8432 | 8432 | 1703 | -6729 |
| `sinf` | 3761 | 3761 | 834 | -2927 |

### armv6-m

Reference programs, release build: code (text + rodata + unwind) and RAM (data + bss + TLS) in bytes; AOR is the size flavour with AOR's string, math (-Os) and fp archives linked in.

| Program | code size | code speed | code AOR | AOR − size | RAM size | RAM AOR | AOR routines linked |
|---|---:|---:|---:|---:|---:|---:|---|
| hello-puts | 2155 | 2243 | 2155 | 0 | 348 | 348 | — |
| hello-printf-int | 7761 | 8201 | 7761 | 0 | 348 | 348 | — |
| hello-printf-float | 10366 | 10806 | 10366 | 0 | 348 | 348 | — |
| malloc-heavy | 2206 | 2330 | 2206 | 0 | 52 | 52 | — |
| math-mix | 33584 | 36060 | 39888 | +6304 | 20 | 20 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpgt`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gedf2`, `__ledf2`, `cosf`, `exp`, `expf`, `log`, `logf`, `pow`, `powf`, `sinf` |
| float-exact | 3744 | 3744 | 4660 | +916 | 20 | 20 | `__aeabi_cfcmpeq`, `__aeabi_cfcmple`, `__aeabi_cfrcmple`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpge`, `__aeabi_fcmpgt`, `__aeabi_fcmple`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gesf2`, `__lesf2` |
| math-float | 9036 | 11460 | 10928 | +1892 | 20 | 20 | `__aeabi_dcmpeq`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpgt`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `cosf`, `expf`, `logf`, `powf`, `sinf` |

What one routine adds to an empty program (code bytes, its tables and helpers included; RAM where any).

| Routine | size flavour | speed flavour | AOR | AOR − size |
|---|---:|---:|---:|---:|
| `strcmp` | 20 | 22 | 3970 | +3950 |
| `cosf` | 3822 | 4774 | 5338 | +1516 |
| `erf` | 13390 | 13390 | 11714 | -1676 |
| `erff` | 4290 | 7122 | 8926 | +4636 |
| `exp` | 6418 | 6418 | 5922 | -496 |
| `exp2` | 6882 | 6882 | 5934 | -948 |
| `exp2f` | 2914 | 3366 | 5030 | +2116 |
| `expf` | 3118 | 3594 | 5042 | +1924 |
| `log` | 8018 | 8018 | 9478 | +1460 |
| `log10f` | 3770 | 3770 | 5094 | +1324 |
| `log2` | 8650 | 8650 | 7418 | -1232 |
| `log2f` | 3574 | 4574 | 5086 | +1512 |
| `logf` | 3770 | 4842 | 5086 | +1316 |
| `pow` | 14762 | 14762 | 13294 | -1468 |
| `powf` | 5366 | 5366 | 6826 | +1460 |
| `sinf` | 3842 | 4794 | 5766 | +1924 |
| `__aeabi_cdcmpeq` | 642 | 642 | 116 | -526 |
| `__aeabi_cdcmple` | 642 | 642 | 120 | -522 |
| `__aeabi_cdrcmple` | 642 | 642 | 120 | -522 |
| `__aeabi_cfcmpeq` | 402 | 402 | 88 | -314 |
| `__aeabi_cfcmple` | 402 | 402 | 88 | -314 |
| `__aeabi_cfrcmple` | 402 | 402 | 88 | -314 |
| `__aeabi_dcmpeq` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmpge` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmpgt` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmple` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmplt` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmpun` | 62 | 62 | 28 | -34 |
| `__aeabi_fadd` | 810 | 810 | 724 | -86 |
| `__aeabi_fcmpeq` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmpge` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmpgt` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmple` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmplt` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmpun` | 42 | 42 | 24 | -18 |
| `__aeabi_fmul` | 402 | 402 | 554 | +152 |
| `__aeabi_fsub` | 826 | 826 | 724 | -102 |
| `__gedf2` | 202 | 202 | 264 | +62 |
| `__gesf2` | 108 | 108 | 180 | +72 |
| `__ledf2` | 202 | 202 | 260 | +58 |
| `__lesf2` | 106 | 106 | 176 | +70 |

### armv6s-m

Reference programs, release build: code (text + rodata + unwind) and RAM (data + bss + TLS) in bytes; AOR is the size flavour with AOR's string, math (-Os) and fp archives linked in.

| Program | code size | code speed | code AOR | AOR − size | RAM size | RAM AOR | AOR routines linked |
|---|---:|---:|---:|---:|---:|---:|---|
| hello-puts | 2155 | 2243 | 2155 | 0 | 348 | 348 | — |
| hello-printf-int | 7761 | 8201 | 7761 | 0 | 348 | 348 | — |
| hello-printf-float | 10366 | 10806 | 10366 | 0 | 348 | 348 | — |
| malloc-heavy | 2206 | 2330 | 2206 | 0 | 52 | 52 | — |
| math-mix | 33584 | 36060 | 39888 | +6304 | 20 | 20 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpgt`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gedf2`, `__ledf2`, `cosf`, `exp`, `expf`, `log`, `logf`, `pow`, `powf`, `sinf` |
| float-exact | 3744 | 3744 | 4660 | +916 | 20 | 20 | `__aeabi_cfcmpeq`, `__aeabi_cfcmple`, `__aeabi_cfrcmple`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpge`, `__aeabi_fcmpgt`, `__aeabi_fcmple`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `__gesf2`, `__lesf2` |
| math-float | 9036 | 11460 | 10928 | +1892 | 20 | 20 | `__aeabi_dcmpeq`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_fadd`, `__aeabi_fcmpeq`, `__aeabi_fcmpgt`, `__aeabi_fcmplt`, `__aeabi_fcmpun`, `__aeabi_fmul`, `__aeabi_frsub`, `__aeabi_fsub`, `cosf`, `expf`, `logf`, `powf`, `sinf` |

What one routine adds to an empty program (code bytes, its tables and helpers included; RAM where any).

| Routine | size flavour | speed flavour | AOR | AOR − size |
|---|---:|---:|---:|---:|
| `strcmp` | 20 | 22 | 3970 | +3950 |
| `cosf` | 3822 | 4774 | 5338 | +1516 |
| `erf` | 13390 | 13390 | 11714 | -1676 |
| `erff` | 4290 | 7122 | 8926 | +4636 |
| `exp` | 6418 | 6418 | 5922 | -496 |
| `exp2` | 6882 | 6882 | 5934 | -948 |
| `exp2f` | 2914 | 3366 | 5030 | +2116 |
| `expf` | 3118 | 3594 | 5042 | +1924 |
| `log` | 8018 | 8018 | 9478 | +1460 |
| `log10f` | 3770 | 3770 | 5094 | +1324 |
| `log2` | 8650 | 8650 | 7418 | -1232 |
| `log2f` | 3574 | 4574 | 5086 | +1512 |
| `logf` | 3770 | 4842 | 5086 | +1316 |
| `pow` | 14762 | 14762 | 13294 | -1468 |
| `powf` | 5366 | 5366 | 6826 | +1460 |
| `sinf` | 3842 | 4794 | 5766 | +1924 |
| `__aeabi_cdcmpeq` | 642 | 642 | 116 | -526 |
| `__aeabi_cdcmple` | 642 | 642 | 120 | -522 |
| `__aeabi_cdrcmple` | 642 | 642 | 120 | -522 |
| `__aeabi_cfcmpeq` | 402 | 402 | 88 | -314 |
| `__aeabi_cfcmple` | 402 | 402 | 88 | -314 |
| `__aeabi_cfrcmple` | 402 | 402 | 88 | -314 |
| `__aeabi_dcmpeq` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmpge` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmpgt` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmple` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmplt` | 642 | 642 | 180 | -462 |
| `__aeabi_dcmpun` | 62 | 62 | 28 | -34 |
| `__aeabi_fadd` | 810 | 810 | 724 | -86 |
| `__aeabi_fcmpeq` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmpge` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmpgt` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmple` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmplt` | 402 | 402 | 136 | -266 |
| `__aeabi_fcmpun` | 42 | 42 | 24 | -18 |
| `__aeabi_fmul` | 402 | 402 | 554 | +152 |
| `__aeabi_fsub` | 826 | 826 | 724 | -102 |
| `__gedf2` | 202 | 202 | 264 | +62 |
| `__gesf2` | 108 | 108 | 180 | +72 |
| `__ledf2` | 202 | 202 | 260 | +58 |
| `__lesf2` | 106 | 106 | 176 | +70 |

### armv7e-m

Reference programs, release build: code (text + rodata + unwind) and RAM (data + bss + TLS) in bytes; AOR is the size flavour with AOR's string, math (-Os) and fp archives linked in.

| Program | code size | code speed | code AOR | AOR − size | RAM size | RAM AOR | AOR routines linked |
|---|---:|---:|---:|---:|---:|---:|---|
| hello-puts | 1887 | 2007 | 5919 | +4032 | 348 | 348 | `memchr`, `strlen` |
| hello-printf-int | 7169 | 7697 | 7365 | +196 | 348 | 348 | `memchr`, `memset` |
| hello-printf-float | 9154 | 9682 | 8990 | -164 | 348 | 348 | `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_i2d`, `memchr`, `memset` |
| malloc-heavy | 1914 | 2094 | 5880 | +3966 | 52 | 52 | `memset`, `strlen` |
| math-mix | 26844 | 26908 | 35788 | +8944 | 20 | 20 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_dadd`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `__gedf2`, `__ledf2`, `cosf`, `exp`, `expf`, `log`, `logf`, `memset`, `pow`, `powf`, `sinf` |
| float-exact | 2048 | 2048 | 2048 | 0 | 20 | 20 | — |
| math-float | 4818 | 4818 | 7902 | +3084 | 20 | 20 | `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_dadd`, `__aeabi_dcmpeq`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `cosf`, `expf`, `logf`, `powf`, `sinf` |

What one routine adds to an empty program (code bytes, its tables and helpers included; RAM where any).

| Routine | size flavour | speed flavour | AOR | AOR − size |
|---|---:|---:|---:|---:|
| `memchr` | 88 | 88 | 248 | +160 |
| `memset` | 48 | 108 | 88 | +40 |
| `strlen` | 44 | 44 | 4122 | +4078 |
| `cosf` | 1510 | 1510 | 3464 | +1954 |
| `erf` | 11030 | 11030 | 10940 | -90 |
| `erff` | 1360 | 1360 | 3180 | +1820 |
| `exp` | 5094 | 5094 | 5132 | +38 |
| `exp2` | 5562 | 5562 | 5132 | -430 |
| `exp2f` | 856 | 856 | 2844 | +1988 |
| `expf` | 948 | 948 | 2864 | +1916 |
| `log` | 5458 | 5458 | 7892 | +2434 |
| `log10f` | 1070 | 1070 | 2860 | +1790 |
| `log2` | 6142 | 6142 | 5824 | -318 |
| `log2f` | 1018 | 1018 | 2864 | +1846 |
| `logf` | 1070 | 1070 | 2852 | +1782 |
| `pow` | 12378 | 12378 | 11536 | -842 |
| `powf` | 2268 | 2268 | 4436 | +2168 |
| `sinf` | 1538 | 1538 | 3468 | +1930 |
| `__aeabi_cdcmpeq` | 274 | 274 | 124 | -150 |
| `__aeabi_cdcmple` | 274 | 274 | 128 | -146 |
| `__aeabi_cdrcmple` | 274 | 274 | 128 | -146 |
| `__aeabi_d2f` | 162 | 162 | 224 | +62 |
| `__aeabi_d2iz` | 82 | 82 | 80 | -2 |
| `__aeabi_d2lz` | 1926 | 1926 | 140 | -1786 |
| `__aeabi_d2uiz` | 66 | 66 | 72 | +6 |
| `__aeabi_d2ulz` | 1610 | 1610 | 92 | -1518 |
| `__aeabi_dadd` | 890 | 890 | 1308 | +418 |
| `__aeabi_dcmpeq` | 274 | 274 | 168 | -106 |
| `__aeabi_dcmpge` | 274 | 274 | 180 | -94 |
| `__aeabi_dcmpgt` | 274 | 274 | 180 | -94 |
| `__aeabi_dcmple` | 274 | 274 | 180 | -94 |
| `__aeabi_dcmplt` | 274 | 274 | 180 | -94 |
| `__aeabi_dcmpun` | 46 | 46 | 32 | -14 |
| `__aeabi_ddiv` | 1062 | 1062 | 1164 | +102 |
| `__aeabi_dmul` | 598 | 598 | 812 | +214 |
| `__aeabi_drsub` | 890 | 890 | 1308 | +418 |
| `__aeabi_dsub` | 890 | 890 | 1308 | +418 |
| `__aeabi_f2d` | 890 | 890 | 104 | -786 |
| `__aeabi_f2lz` | 1654 | 1654 | 124 | -1530 |
| `__aeabi_f2ulz` | 1614 | 1614 | 76 | -1538 |
| `__aeabi_fadd` | 542 | 542 | 716 | +174 |
| `__aeabi_frsub` | 542 | 542 | 716 | +174 |
| `__aeabi_fsub` | 542 | 542 | 716 | +174 |
| `__aeabi_i2d` | 890 | 890 | 48 | -842 |
| `__aeabi_i2f` | 542 | 542 | 48 | -494 |
| `__aeabi_l2d` | 890 | 890 | 164 | -726 |
| `__aeabi_l2f` | 542 | 542 | 100 | -442 |
| `__aeabi_ui2d` | 890 | 890 | 40 | -850 |
| `__aeabi_ui2f` | 542 | 542 | 40 | -502 |
| `__aeabi_ul2d` | 890 | 890 | 140 | -750 |
| `__aeabi_ul2f` | 542 | 542 | 100 | -442 |
| `__gedf2` | 274 | 274 | 216 | -58 |
| `__ledf2` | 274 | 274 | 216 | -58 |

### armv7e-m-dp

Reference programs, release build: code (text + rodata + unwind) and RAM (data + bss + TLS) in bytes; AOR is the size flavour with AOR's string, math (-Os) and fp archives linked in.

| Program | code size | code speed | code AOR | AOR − size | RAM size | RAM AOR | AOR routines linked |
|---|---:|---:|---:|---:|---:|---:|---|
| hello-puts | 1887 | 2007 | 5919 | +4032 | 348 | 348 | `memchr`, `strlen` |
| hello-printf-int | 7204 | 7732 | 7400 | +196 | 348 | 348 | `memchr`, `memset` |
| hello-printf-float | 7228 | 7756 | 7428 | +200 | 348 | 348 | `memchr`, `memset` |
| malloc-heavy | 1914 | 2094 | 5880 | +3966 | 52 | 52 | `memset`, `strlen` |
| math-mix | 11662 | 11726 | 21014 | +9352 | 20 | 20 | `__aeabi_l2d`, `cosf`, `exp`, `expf`, `log`, `logf`, `memset`, `pow`, `powf`, `sinf` |
| float-exact | 1688 | 1688 | 1688 | 0 | 20 | 20 | — |
| math-float | 9078 | 9134 | 4106 | -4972 | 20 | 20 | `__aeabi_l2d`, `cosf`, `expf`, `logf`, `powf`, `sinf` |

What one routine adds to an empty program (code bytes, its tables and helpers included; RAM where any).

| Routine | size flavour | speed flavour | AOR | AOR − size |
|---|---:|---:|---:|---:|
| `memchr` | 88 | 88 | 248 | +160 |
| `memset` | 48 | 108 | 88 | +40 |
| `strlen` | 44 | 44 | 4122 | +4078 |
| `cosf` | 2092 | 2148 | 876 | -1216 |
| `erf` | 5466 | 5466 | 1924 | -3542 |
| `erff` | 5482 | 5482 | 904 | -4578 |
| `exp` | 2292 | 2292 | 2708 | +416 |
| `exp2` | 2384 | 2384 | 2708 | +324 |
| `exp2f` | 2400 | 2400 | 572 | -1828 |
| `expf` | 2308 | 2308 | 592 | -1716 |
| `log` | 2264 | 2264 | 2744 | +480 |
| `log10f` | 2344 | 2344 | 552 | -1792 |
| `log2` | 2646 | 2646 | 1728 | -918 |
| `log2f` | 2662 | 2662 | 536 | -2126 |
| `logf` | 2280 | 2280 | 544 | -1736 |
| `pow` | 5518 | 5518 | 7716 | +2198 |
| `powf` | 5534 | 5534 | 1452 | -4082 |
| `sinf` | 2476 | 2532 | 880 | -1596 |
| `__aeabi_d2lz` | 110 | 110 | 140 | +30 |
| `__aeabi_d2ulz` | 70 | 70 | 92 | +22 |
| `__aeabi_dadd` | 890 | 890 | 1308 | +418 |
| `__aeabi_drsub` | 890 | 890 | 1308 | +418 |
| `__aeabi_dsub` | 890 | 890 | 1308 | +418 |
| `__aeabi_f2d` | 890 | 890 | 104 | -786 |
| `__aeabi_f2lz` | 110 | 110 | 124 | +14 |
| `__aeabi_f2ulz` | 70 | 70 | 76 | +6 |
| `__aeabi_fadd` | 542 | 542 | 716 | +174 |
| `__aeabi_frsub` | 542 | 542 | 716 | +174 |
| `__aeabi_fsub` | 542 | 542 | 716 | +174 |
| `__aeabi_i2d` | 890 | 890 | 48 | -842 |
| `__aeabi_i2f` | 542 | 542 | 48 | -494 |
| `__aeabi_l2d` | 890 | 890 | 164 | -726 |
| `__aeabi_l2f` | 542 | 542 | 100 | -442 |
| `__aeabi_ui2d` | 890 | 890 | 40 | -850 |
| `__aeabi_ui2f` | 542 | 542 | 40 | -502 |
| `__aeabi_ul2d` | 890 | 890 | 140 | -750 |
| `__aeabi_ul2f` | 542 | 542 | 100 | -442 |

### armv8-m.main

Reference programs, release build: code (text + rodata + unwind) and RAM (data + bss + TLS) in bytes; AOR is the size flavour with AOR's string, math (-Os) and fp archives linked in.

| Program | code size | code speed | code AOR | AOR − size | RAM size | RAM AOR | AOR routines linked |
|---|---:|---:|---:|---:|---:|---:|---|
| hello-puts | 1871 | 1991 | 5907 | +4036 | 348 | 348 | `memchr`, `strlen` |
| hello-printf-int | 7153 | 7673 | 7361 | +208 | 348 | 348 | `memchr`, `memset` |
| hello-printf-float | 9138 | 9658 | 8962 | -176 | 348 | 348 | `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_i2d`, `memchr`, `memset` |
| malloc-heavy | 1898 | 2078 | 5868 | +3970 | 52 | 52 | `memset`, `strlen` |
| math-mix | 26772 | 26828 | 35780 | +9008 | 20 | 20 | `__aeabi_cdcmpeq`, `__aeabi_cdcmple`, `__aeabi_cdrcmple`, `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_dadd`, `__aeabi_dcmpeq`, `__aeabi_dcmpge`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dcmplt`, `__aeabi_dcmpun`, `__aeabi_ddiv`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `__aeabi_ui2d`, `__aeabi_ul2d`, `__gedf2`, `__ledf2`, `cosf`, `exp`, `expf`, `log`, `logf`, `memset`, `pow`, `powf`, `sinf` |
| float-exact | 1612 | 1612 | 1612 | 0 | 20 | 20 | — |
| math-float | 4750 | 4750 | 7892 | +3142 | 20 | 20 | `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_dadd`, `__aeabi_dcmpeq`, `__aeabi_dcmpgt`, `__aeabi_dcmple`, `__aeabi_dmul`, `__aeabi_drsub`, `__aeabi_dsub`, `__aeabi_f2d`, `__aeabi_i2d`, `__aeabi_l2d`, `cosf`, `expf`, `logf`, `powf`, `sinf` |

What one routine adds to an empty program (code bytes, its tables and helpers included; RAM where any).

| Routine | size flavour | speed flavour | AOR | AOR − size |
|---|---:|---:|---:|---:|
| `memchr` | 86 | 86 | 248 | +162 |
| `memset` | 48 | 108 | 88 | +40 |
| `strlen` | 44 | 44 | 4116 | +4072 |
| `cosf` | 1508 | 1508 | 3460 | +1952 |
| `erf` | 11028 | 11028 | 10926 | -102 |
| `erff` | 1362 | 1362 | 3172 | +1810 |
| `exp` | 5092 | 5092 | 5124 | +32 |
| `exp2` | 5560 | 5560 | 5124 | -436 |
| `exp2f` | 854 | 854 | 2840 | +1986 |
| `expf` | 946 | 946 | 2860 | +1914 |
| `log` | 5456 | 5456 | 7884 | +2428 |
| `log10f` | 1068 | 1068 | 2852 | +1784 |
| `log2` | 6140 | 6140 | 5816 | -324 |
| `log2f` | 1016 | 1016 | 2856 | +1840 |
| `logf` | 1068 | 1068 | 2844 | +1776 |
| `pow` | 12376 | 12376 | 11528 | -848 |
| `powf` | 2202 | 2202 | 4432 | +2230 |
| `sinf` | 1536 | 1536 | 3464 | +1928 |
| `__aeabi_cdcmpeq` | 272 | 272 | 124 | -148 |
| `__aeabi_cdcmple` | 272 | 272 | 128 | -144 |
| `__aeabi_cdrcmple` | 272 | 272 | 128 | -144 |
| `__aeabi_d2f` | 160 | 160 | 228 | +68 |
| `__aeabi_d2iz` | 80 | 80 | 80 | 0 |
| `__aeabi_d2lz` | 1928 | 1928 | 140 | -1788 |
| `__aeabi_d2uiz` | 64 | 64 | 72 | +8 |
| `__aeabi_d2ulz` | 1608 | 1608 | 92 | -1516 |
| `__aeabi_dadd` | 888 | 888 | 1300 | +412 |
| `__aeabi_dcmpeq` | 272 | 272 | 168 | -104 |
| `__aeabi_dcmpge` | 272 | 272 | 180 | -92 |
| `__aeabi_dcmpgt` | 272 | 272 | 180 | -92 |
| `__aeabi_dcmple` | 272 | 272 | 180 | -92 |
| `__aeabi_dcmplt` | 272 | 272 | 180 | -92 |
| `__aeabi_dcmpun` | 44 | 44 | 32 | -12 |
| `__aeabi_ddiv` | 1060 | 1060 | 1156 | +96 |
| `__aeabi_dmul` | 596 | 596 | 804 | +208 |
| `__aeabi_drsub` | 888 | 888 | 1300 | +412 |
| `__aeabi_dsub` | 888 | 888 | 1300 | +412 |
| `__aeabi_f2d` | 888 | 888 | 104 | -784 |
| `__aeabi_f2lz` | 1652 | 1652 | 124 | -1528 |
| `__aeabi_f2ulz` | 1612 | 1612 | 76 | -1536 |
| `__aeabi_fadd` | 540 | 540 | 716 | +176 |
| `__aeabi_frsub` | 540 | 540 | 716 | +176 |
| `__aeabi_fsub` | 540 | 540 | 716 | +176 |
| `__aeabi_i2d` | 888 | 888 | 48 | -840 |
| `__aeabi_i2f` | 540 | 540 | 48 | -492 |
| `__aeabi_l2d` | 888 | 888 | 164 | -724 |
| `__aeabi_l2f` | 540 | 540 | 100 | -440 |
| `__aeabi_ui2d` | 888 | 888 | 40 | -848 |
| `__aeabi_ui2f` | 540 | 540 | 40 | -500 |
| `__aeabi_ul2d` | 888 | 888 | 140 | -748 |
| `__aeabi_ul2f` | 540 | 540 | 100 | -440 |
| `__gedf2` | 272 | 272 | 216 | -56 |
| `__ledf2` | 272 | 272 | 216 | -56 |

