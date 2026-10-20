# Phase 2 size report

The end of phase 2 (P2.07, PLAN D71). It compares release builds (`-Os`) at
the end of phase 1 (the P2.01 baseline, `docs/size/baseline-*.json` at
commit 3fee3af) with today, using the same `tools/scripts/size-report`
(D51). It then compares the libc share of every reference program with
picolibc 1.8.6 and newlib-nano 4.4 (`size-report compare`, same compiler and
flags). The step-by-step history is in `docs/size/<arch>.md`.

How to read the figures:
- *code* is text + rodata + unwind;
- *RAM* is data + bss + TLS;
- for images, both are of the whole program as linked through `port/linux`;
- for `libc.a`, they are the sum over every member. The archive grows when
  functions gain their own implementations, without any program linking
  more.

## Phase 1 → phase 2

| Arch | Program | code, P1 | code, P2 | change | RAM, P1 | RAM, P2 |
|---|---|---:|---:|---:|---:|---:|
| armv6-m | `libc.a` | 121409 | 167130 | +37.7 % | 1192 | 1658 |
| armv6-m | hello-puts | 2887 | 2163 | -25.1 % | 364 | 388 |
| armv6-m | hello-printf-int | 8829 | 7769 | -12.0 % | 364 | 388 |
| armv6-m | hello-printf-float | 11374 | 10374 | -8.8 % | 364 | 388 |
| armv6-m | malloc-heavy | 2332 | 2214 | -5.1 % | 236 | 88 |
| armv6-m | math-mix | 30720 | 34116 | +11.1 % | 204 | 20 |
| armv6-m | hello-printf-int-nofloat | — | 4987 | new | — | 388 |
| armv6-m | hello-printf-int-minimal | — | 4719 | new | — | 388 |
| armv6-m | malloc-heavy-tiny | — | 2134 | new | — | 84 |
| armv6-m | float-exact | — | 3908 | new | — | 20 |
| armv6-m | math-float | — | 9640 | new | — | 20 |
| armv6s-m | `libc.a` | 121409 | 167130 | +37.7 % | 1192 | 1658 |
| armv6s-m | hello-puts | 2887 | 2163 | -25.1 % | 364 | 388 |
| armv6s-m | hello-printf-int | 8829 | 7769 | -12.0 % | 364 | 388 |
| armv6s-m | hello-printf-float | 11374 | 10374 | -8.8 % | 364 | 388 |
| armv6s-m | malloc-heavy | 2332 | 2214 | -5.1 % | 236 | 88 |
| armv6s-m | math-mix | 30720 | 34116 | +11.1 % | 204 | 20 |
| armv6s-m | hello-printf-int-nofloat | — | 4987 | new | — | 388 |
| armv6s-m | hello-printf-int-minimal | — | 4719 | new | — | 388 |
| armv6s-m | malloc-heavy-tiny | — | 2134 | new | — | 84 |
| armv6s-m | float-exact | — | 3908 | new | — | 20 |
| armv6s-m | math-float | — | 9640 | new | — | 20 |
| armv7e-m | `libc.a` | 121457 | 154198 | +27.0 % | 1192 | 1658 |
| armv7e-m | hello-puts | 2595 | 1903 | -26.7 % | 364 | 388 |
| armv7e-m | hello-printf-int | 8577 | 7185 | -16.2 % | 364 | 388 |
| armv7e-m | hello-printf-float | 10562 | 9170 | -13.2 % | 364 | 388 |
| armv7e-m | malloc-heavy | 2024 | 1934 | -4.4 % | 236 | 88 |
| armv7e-m | math-mix | 23900 | 27312 | +14.3 % | 204 | 20 |
| armv7e-m | hello-printf-int-nofloat | — | 4471 | new | — | 388 |
| armv7e-m | hello-printf-int-minimal | — | 4231 | new | — | 388 |
| armv7e-m | malloc-heavy-tiny | — | 1874 | new | — | 84 |
| armv7e-m | float-exact | — | 2048 | new | — | 20 |
| armv7e-m | math-float | — | 5282 | new | — | 20 |
| armv7e-m-dp | `libc.a` | 86545 | 86790 | +0.3 % | 1192 | 1658 |
| armv7e-m-dp | hello-puts | 2595 | 1903 | -26.7 % | 364 | 388 |
| armv7e-m-dp | hello-printf-int | 8612 | 7220 | -16.2 % | 364 | 388 |
| armv7e-m-dp | hello-printf-float | 8636 | 7244 | -16.1 % | 364 | 388 |
| armv7e-m-dp | malloc-heavy | 2024 | 1934 | -4.4 % | 236 | 88 |
| armv7e-m-dp | math-mix | 13708 | 12302 | -10.3 % | 204 | 20 |
| armv7e-m-dp | hello-printf-int-nofloat | — | 4471 | new | — | 388 |
| armv7e-m-dp | hello-printf-int-minimal | — | 4231 | new | — | 388 |
| armv7e-m-dp | malloc-heavy-tiny | — | 1874 | new | — | 84 |
| armv7e-m-dp | float-exact | — | 1688 | new | — | 20 |
| armv7e-m-dp | math-float | — | 9494 | new | — | 20 |
| armv8-m.main | `libc.a` | 120977 | 153200 | +26.6 % | 1192 | 1658 |
| armv8-m.main | hello-puts | 2579 | 1883 | -27.0 % | 364 | 388 |
| armv8-m.main | hello-printf-int | 8565 | 7165 | -16.3 % | 364 | 388 |
| armv8-m.main | hello-printf-float | 10550 | 9150 | -13.3 % | 364 | 388 |
| armv8-m.main | malloc-heavy | 2008 | 1914 | -4.7 % | 236 | 88 |
| armv8-m.main | math-mix | 23888 | 27232 | +14.0 % | 204 | 20 |
| armv8-m.main | hello-printf-int-nofloat | — | 4439 | new | — | 388 |
| armv8-m.main | hello-printf-int-minimal | — | 4203 | new | — | 388 |
| armv8-m.main | malloc-heavy-tiny | — | 1854 | new | — | 84 |
| armv8-m.main | float-exact | — | 1612 | new | — | 20 |
| armv8-m.main | math-float | — | 5214 | new | — | 20 |
| x86_64 | `libc.a` | 164359 | 116323 | -29.2 % | 2204 | 2662 |
| x86_64 | hello-puts | 4556 | 2384 | -47.7 % | 732 | 532 |
| x86_64 | hello-printf-int | 13373 | 10436 | -22.0 % | 732 | 532 |
| x86_64 | hello-printf-float | 13404 | 10483 | -21.8 % | 732 | 532 |
| x86_64 | malloc-heavy | 3632 | 2601 | -28.4 % | 500 | 196 |
| x86_64 | math-mix | 19322 | 16623 | -14.0 % | 432 | 40 |
| x86_64 | hello-printf-int-nofloat | — | 6534 | new | — | 532 |
| x86_64 | hello-printf-int-minimal | — | 6154 | new | — | 532 |
| x86_64 | malloc-heavy-tiny | — | 2484 | new | — | 188 |
| x86_64 | float-exact | — | 3256 | new | — | 40 |
| x86_64 | math-float | — | 13045 | new | — | 40 |

Notes:
- **The printf and puts programs** are 12–27 % smaller on Arm (22–48 % on
  x86_64), from:
  - pull-in chains broken at link time (D52);
  - the FILE and printf rework and `__libc_udivmod64` (D53, D54);
  - the string kernels (D69, D70).
- **The flavour archives** go further: armv7e-m `hello-printf-int` is
  4471 bytes with `libc-nofloat.a` and 4231 with `libc-minimal-printf.a`,
  against 8577 at the end of phase 1.
- **RAM in the stdio programs** grew by 24 bytes: `stdin` and `stdout`
  have static buffers since D53 instead of 256 heap bytes at first use, so
  RAM that the old figure did not count now shows. `malloc-heavy` and
  `math-mix` lost their TLS and data.
- **`math-mix` grew by 11–14 % on the four soft-fenv cores.** The program
  calls float and double transcendental functions together.
  - Since D57 the float ones have their own float-float kernels instead of
    wrapping the double ones, so a mixed program links both sets.
  - Float-only programs are where the phase 2 target is (§1). There
    `math-float` went from 18044 to 5282 bytes on armv7e-m during P2.05,
    and it links no libgcc.
  - A flavour archive with the float functions as double wrappers would
    serve mixed programs. It is PLAN Q15.
  - On armv7e-m-dp, where the double functions are hardware, `math-mix`
    shrank by 10 %.
- **`libc.a` itself** grew on the soft-fenv cores (+27 to +38 %). It now
  holds about 120 float-float functions and the float complex layer, which
  a program links only when it calls them. On x86_64 it shrank by 29 %
  (no unwind tables, D52).

## Against picolibc and newlib-nano (libc share; image code includes each library's OS layer)

| Arch | Program | rtoskit libc code | picolibc libc code | newlib-nano libc code | rtoskit libc RAM | picolibc libc RAM | newlib-nano libc RAM | rtoskit image code | picolibc image code | newlib-nano image code |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| armv6-m | hello-puts | 1380 | 206 | 3620 | 300 | 68 | 564 | 2163 | 364 | 4081 |
| armv6-m | hello-printf-int | 6533 | 6633 | 6772 | 300 | 68 | 564 | 7769 | 7660 | 7576 |
| armv6-m | hello-printf-float | 6533 | 6633 | 18031 | 300 | 68 | 928 | 10374 | 10204 | 25241 |
| armv6-m | malloc-heavy | 1274 | 1016 | 1916 | 16 | 89 | 552 | 2214 | 1280 | 2432 |
| armv6-m | math-mix | 25646 | 19496 | 24849 | 0 | 68 | 540 | 34116 | 29984 | 35352 |
| armv6-m | float-exact | 1444 | 1808 | 2708 | 0 | 68 | 540 | 3908 | 4792 | 6516 |
| armv6-m | math-float | 6504 | 7338 | 9597 | 0 | 68 | 540 | 9640 | 11040 | 13312 |
| armv6s-m | hello-puts | 1380 | 206 | 3620 | 300 | 68 | 564 | 2163 | 364 | 4081 |
| armv6s-m | hello-printf-int | 6533 | 6633 | 6772 | 300 | 68 | 564 | 7769 | 7660 | 7576 |
| armv6s-m | hello-printf-float | 6533 | 6633 | 18031 | 300 | 68 | 928 | 10374 | 10204 | 25241 |
| armv6s-m | malloc-heavy | 1274 | 1016 | 1916 | 16 | 89 | 552 | 2214 | 1280 | 2432 |
| armv6s-m | math-mix | 25646 | 19496 | 24849 | 0 | 68 | 540 | 34116 | 29984 | 35352 |
| armv6s-m | float-exact | 1444 | 1808 | 2708 | 0 | 68 | 540 | 3908 | 4792 | 6516 |
| armv6s-m | math-float | 6504 | 7338 | 9597 | 0 | 68 | 540 | 9640 | 11040 | 13312 |
| armv7e-m | hello-puts | 1148 | 212 | 3488 | 300 | 68 | 564 | 1903 | 396 | 3929 |
| armv7e-m | hello-printf-int | 6397 | 6381 | 6896 | 300 | 68 | 564 | 7185 | 7344 | 7365 |
| armv7e-m | hello-printf-float | 6397 | 6381 | 17575 | 300 | 68 | 928 | 9170 | 9328 | 20461 |
| armv7e-m | malloc-heavy | 1016 | 960 | 2156 | 16 | 89 | 552 | 1934 | 1264 | 2692 |
| armv7e-m | math-mix | 24160 | 19154 | 23709 | 0 | 68 | 540 | 27312 | 22296 | 26888 |
| armv7e-m | float-exact | 1518 | 2020 | 2896 | 0 | 68 | 540 | 2048 | 2400 | 3464 |
| armv7e-m | math-float | 4870 | 6444 | 8237 | 0 | 68 | 540 | 5282 | 6848 | 8684 |
| armv7e-m-dp | hello-puts | 1148 | 212 | 3488 | 300 | 68 | 564 | 1903 | 396 | 3929 |
| armv7e-m-dp | hello-printf-int | 6429 | 6381 | 6896 | 300 | 68 | 564 | 7220 | 7344 | 7365 |
| armv7e-m-dp | hello-printf-float | 6429 | 6381 | 16983 | 300 | 68 | 928 | 7244 | 7360 | 17541 |
| armv7e-m-dp | malloc-heavy | 1016 | 960 | 2156 | 16 | 89 | 552 | 1934 | 1264 | 2692 |
| armv7e-m-dp | math-mix | 11614 | 15044 | 22328 | 0 | 68 | 540 | 12302 | 15536 | 23924 |
| armv7e-m-dp | float-exact | 1198 | 1496 | 2272 | 0 | 68 | 540 | 1688 | 1832 | 2800 |
| armv7e-m-dp | math-float | 9048 | 6280 | 4384 | 0 | 68 | 540 | 9494 | 6528 | 5720 |
| armv8-m.main | hello-puts | 1140 | 210 | 3456 | 300 | 68 | 564 | 1883 | 396 | 3901 |
| armv8-m.main | hello-printf-int | 6391 | 6353 | 6662 | 300 | 68 | 564 | 7165 | 7320 | 7133 |
| armv8-m.main | hello-printf-float | 6391 | 6353 | 17217 | 300 | 68 | 928 | 9150 | 9320 | 20109 |
| armv8-m.main | malloc-heavy | 1010 | 958 | 1890 | 16 | 89 | 552 | 1914 | 1264 | 2400 |
| armv8-m.main | math-mix | 24082 | 19068 | 23637 | 0 | 68 | 540 | 27232 | 22208 | 26832 |
| armv8-m.main | float-exact | 1122 | 1490 | 2272 | 0 | 68 | 540 | 1612 | 1832 | 2800 |
| armv8-m.main | math-float | 4802 | 6342 | 8169 | 0 | 68 | 540 | 5214 | 6760 | 8616 |

Notes:
- **Float programs** (`float-exact`, `math-float`): rtoskit's libc share is
  the smallest of the three on armv6-m, armv7e-m and armv8-m.main:
  - `math-float` armv7e-m: 4870 bytes against picolibc's 6444 and
    newlib-nano's 8237;
  - and its float functions are correctly rounded nearly always,
    where picolibc's float-only code is within about 1 ulp.
  - On armv7e-m-dp the float transcendental functions are the double ones
    (hardware double, full `<fenv.h>`), and both other libraries are
    smaller there.
- **`hello-printf-int`:** the libc share is within 1 % of picolibc on the
  Thumb-2 cores (smaller on armv6-m). The images are smaller than
  picolibc's on every Thumb-2 core and than newlib-nano's on armv7e-m and
  armv7e-m-dp (armv8-m.main: 7165 against 7133). The flavour archives
  roughly halve it.
- **`hello-printf-float`:** about the same as picolibc, and 60 % smaller
  than newlib-nano with `_printf_float`.
- **`hello-puts`:** picolibc writes unbuffered through semihosting. rtoskit
  goes through a buffered, locked `FILE`, so it stays larger, as at the end
  of phase 1.
- **`math-mix`:** see above (Q15).
- **RAM:** the stdio programs use 300 bytes of libc RAM against
  picolibc's 68: `stdout`'s static 256-byte buffer (`BUFSIZ`) plus its FILE
  (D53). The non-stdio
  programs use 0–16 bytes, below both.

## The size budget (D71)

`docs/size/budget.json` locks the code and RAM of every reference program
on every Arm core at these figures, with x86_64 excluded.
`tools/scripts/size-report budget` checks a release build against it and
exits 1 when an image is over:
- `--flavour size` (the default) allows +0 %;
- `--flavour speed` allows +10 %. That is N for the phase 3 speed flavour:
  room for word-at-a-time and unrolled kernels and tables, small enough
  that a whole subsystem doubling shows.

Raising the budget takes a PLAN decision, as any baseline change does
(D51).
