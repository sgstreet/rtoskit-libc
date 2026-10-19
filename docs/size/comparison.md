# Size comparison with picolibc and newlib-nano (P2.01)

For context only; this is not a gate (phase 2, PLAN D51). Produced by
`tools/scripts/size-report compare` after a release build. It builds the
`test/size` reference programs with the same arm-none-eabi GCC 16.2 and the same
flags (`-Os -ffunction-sections -fdata-sections`, `--gc-sections`, `-std=gnu23`)
against the Ubuntu 24.04 packages, which are pinned by SHA-256:
- picolibc 1.8.6, with its crt0, linker script and `--oslib=semihost`;
- newlib 4.4.0.20231231, with `libc_nano.a`, `libnosys.a` and newlib's crt0.

How to read the table:
- The *libc* columns are each library's share of the image, taken from the
  link map: `libc.a`/`libc_nano.a` plus `libm.a`.
- *libc RAM* is data + bss + TLS.
- The *image* columns are whole programs. They also contain each library's
  OS layer, which differs: Linux system calls through `port/linux`,
  semihosting, or `libnosys` stubs.
- Each library keeps its default `printf`. picolibc's is the full double
  version, so its integer and float programs are the same size.
  newlib-nano's default is integer-only, and `hello-printf-float` adds
  `-u _printf_float`.
- rtoskit always links its float formatter today (D13's stub archive
  arrives in P2.03).

Observations at the end of phase 1 (libc shares):
- `hello-puts`: picolibc's `puts` writes unbuffered through semihosting
  (about 210 bytes). rtoskit's goes through a buffered, locked `FILE`
  (1730 to 1996 bytes).
- `hello-printf-int`: rtoskit is 7 to 9 % larger than picolibc and 0.5 to
  5 % larger than newlib-nano. It carries the float formatter that P2.03
  makes optional.
- `hello-printf-float`: rtoskit is 7 to 9 % larger than picolibc and about 60 %
  smaller than newlib-nano with `_printf_float`.
- `math-mix`:
  - 3.6 to 5.8 % larger than picolibc on the soft-float and
    single-precision cores;
  - 22 % smaller on armv7e-m-dp, where the double functions use the FPU
    (D46).
- RAM in the stdio programs is 228 bytes against picolibc's 68.

| Arch | Program | rtoskit libc code | picolibc libc code | newlib-nano libc code | rtoskit libc RAM | picolibc libc RAM | newlib-nano libc RAM | rtoskit image code | picolibc image code | newlib-nano image code |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| armv6-m | hello-puts | 1996 | 206 | 3620 | 228 | 68 | 564 | 2887 | 364 | 4081 |
| armv6-m | hello-printf-int | 7105 | 6633 | 6772 | 228 | 68 | 564 | 8829 | 7660 | 7576 |
| armv6-m | hello-printf-float | 7105 | 6633 | 18031 | 228 | 68 | 928 | 11374 | 10204 | 25241 |
| armv6-m | malloc-heavy | 1390 | 1016 | 1916 | 148 | 89 | 552 | 2332 | 1280 | 2432 |
| armv6-m | math-mix | 20198 | 19496 | 24849 | 132 | 68 | 540 | 30720 | 29984 | 35352 |
| armv6s-m | hello-puts | 1996 | 206 | 3620 | 228 | 68 | 564 | 2887 | 364 | 4081 |
| armv6s-m | hello-printf-int | 7105 | 6633 | 6772 | 228 | 68 | 564 | 8829 | 7660 | 7576 |
| armv6s-m | hello-printf-float | 7105 | 6633 | 18031 | 228 | 68 | 928 | 11374 | 10204 | 25241 |
| armv6s-m | malloc-heavy | 1390 | 1016 | 1916 | 148 | 89 | 552 | 2332 | 1280 | 2432 |
| armv6s-m | math-mix | 20198 | 19496 | 24849 | 132 | 68 | 540 | 30720 | 29984 | 35352 |
| armv7e-m | hello-puts | 1738 | 212 | 3488 | 228 | 68 | 564 | 2595 | 396 | 3929 |
| armv7e-m | hello-printf-int | 6931 | 6381 | 6896 | 228 | 68 | 564 | 8577 | 7344 | 7365 |
| armv7e-m | hello-printf-float | 6931 | 6381 | 17575 | 228 | 68 | 928 | 10562 | 9328 | 20461 |
| armv7e-m | malloc-heavy | 1106 | 960 | 2156 | 148 | 89 | 552 | 2024 | 1264 | 2692 |
| armv7e-m | math-mix | 20188 | 19154 | 23709 | 132 | 68 | 540 | 23900 | 22296 | 26888 |
| armv7e-m-dp | hello-puts | 1738 | 212 | 3488 | 228 | 68 | 564 | 2595 | 396 | 3929 |
| armv7e-m-dp | hello-printf-int | 6963 | 6381 | 6896 | 228 | 68 | 564 | 8612 | 7344 | 7365 |
| armv7e-m-dp | hello-printf-float | 6963 | 6381 | 16983 | 228 | 68 | 928 | 8636 | 7360 | 17541 |
| armv7e-m-dp | malloc-heavy | 1106 | 960 | 2156 | 148 | 89 | 552 | 2024 | 1264 | 2692 |
| armv7e-m-dp | math-mix | 11706 | 15044 | 22328 | 132 | 68 | 540 | 13708 | 15536 | 23924 |
| armv8-m.main | hello-puts | 1730 | 210 | 3456 | 228 | 68 | 564 | 2579 | 396 | 3901 |
| armv8-m.main | hello-printf-int | 6929 | 6353 | 6662 | 228 | 68 | 564 | 8565 | 7320 | 7133 |
| armv8-m.main | hello-printf-float | 6929 | 6353 | 17217 | 228 | 68 | 928 | 10550 | 9320 | 20109 |
| armv8-m.main | malloc-heavy | 1100 | 958 | 1890 | 148 | 89 | 552 | 2008 | 1264 | 2400 |
| armv8-m.main | math-mix | 20180 | 19068 | 23637 | 132 | 68 | 540 | 23888 | 22208 | 26832 |
