# Research: stdio and printf flavours

Step: P2.03. Decisions: PLAN D13 (feature trimming at link time), D53.

## Sources reviewed

- picolibc, `doc/printf.md`
  (<https://github.com/picolibc/picolibc/blob/main/doc/printf.md>):
  - five tiers: `d` (default, double), `f` (float only), `l` (long long, no
    floating point), `i` (integer up to long) and `m` (minimal: width,
    precision and flags parsed but ignored);
  - selected with `--printf=`, which makes the specs file pass
    `--defsym=vfprintf=__d_vfprintf` (and so on) to the linker;
  - with no floating point, a float argument prints as `*float*`, and the
    minimal tier prints an unsupported `%g` literally;
  - `defsym` does not survive LTO;
  - reported text sizes for one program on arm-none-eabi: 7544 (`d`), 6856,
    2100, 1924 and 1420 bytes (`m`).
- newlib-nano: `printf`'s float conversion lives in a separate object reached
  through the weak symbol `_printf_float`, linked only with
  `-u _printf_float`. Without it, a float conversion prints nothing
  (<https://blueprints.launchpad.net/gcc-arm-embedded/+spec/nano-float-printf>,
  ST and NXP community threads).
- Zephyr `cbprintf` (*Formatted Output* in the Zephyr documentation):
  Kconfig options `CBPRINTF_NANO`, `CBPRINTF_REDUCED_INTEGRAL` (no 64-bit
  values), `CBPRINTF_FP_SUPPORT` and `CBPRINTF_N_SPECIFIER`. The minimal
  libc drops `%n`, most flags, precision and floating point.
- musl and picolibc FILE layouts (structure only), and C23 7.23.3 (the
  buffering rules), 7.23.5.6 (`setvbuf`: with a null `buf` the size "may
  determine" the buffer's size, so it may be ignored) and 7.23.6.

## Trade-offs

| Question | Options | Chosen |
|---|---|---|
| How a flavour is selected | (a) A small archive linked ahead of `libc.a`. (b) `--defsym` aliases (picolibc). (c) Complete archives that differ in a few members. | (c) `libc-nofloat.a` and `libc-minimal-printf.a`. (a) cannot work: once `libc.a`'s `__libc_format` is extracted, the linker satisfies its reference to `__libc_format_float` from `libc.a` itself, even inside a group. (b) needs a specs file and breaks with LTO. A complete archive is chosen by naming it (`-lc-nofloat` instead of `-lc`). |
| A floating conversion in `libc-nofloat.a` | print `*float*` (picolibc), print nothing (newlib-nano), or fail | Fail with `EINVAL`, as every other invalid specification does (D28). `scanf` gets a matching failure with `EINVAL` (D41). Silent wrong output is worse than an error. |
| The minimal tier | picolibc's `m` (no width, precision or flags); Zephyr's reduced integral | Keep every flag, the width and the precision. Leave out `%n` and the length modifiers whose type is wider than `long` (`ll`, `j`, `w64`, `wf64`, and `z`/`t` where wider). Then the digits need only `unsigned long` division, and the 64-bit helpers (`__aeabi_uldivmod`, `__udivmoddi4`, ...) disappear from 32-bit images. It includes `libc-nofloat.a`'s stubs. The refused forms fail with `EINVAL`. |
| Granularity | One minimal `__libc_format` copy | Three small replaceable units (D6, one function per file), shared by the full engine: `__libc_format_argument` (fetch by length), `__libc_format_digits` and `__libc_format_count` (`%n`). For `scanf`, `__libc_scan_store` and `__libc_scan_count`. The minimal flavour replaces exactly these. |

## The FILE and its buffers

- `struct __libc_file` loses `prev`: 36 → 32 bytes on the 32-bit targets.
  The list of opened streams is singly linked. Unlinking searches at most
  `FOPEN_MAX` entries under the list lock.
- `stdin` and `stdout` start with a static `BUFSIZ` buffer, defined in the
  same object as the stream, so it is linked only when the program names
  that stream.
  - `__libc_file_setup` names `malloc` weakly. `fopen`/`tmpfile` link
    `calloc`, and through it `malloc`, for the FILE itself, so their
    streams still get a heap buffer at the first transfer. A program that
    writes only to `stdout` no longer links the allocator: 368 bytes of code
    on armv6-m.
  - The RAM moves rather than grows: the 256-byte buffer was on the heap
    (malloc'd at the first `puts`, plus the allocator's header), and is now
    in `.bss`, which `size-report` counts. Static RAM rises by 224 to 256
    bytes in the stdio images; heap use falls by about 264 bytes.
- `setvbuf` with a null `buf` keeps the stream's own buffer (static or
  already allocated) and ignores the size (7.23.5.6p2 allows it).
  - A caller's buffer from an earlier call is always given up.
  - `_IONBF` switches the stream to its one-byte buffer.
  - The caller's buffer is tracked with a new flag, `__LIBC_FILE_USER_BUFFER`.
- `stderr` stays unbuffered, with no buffer.

## Measured effect (armv6-m / armv7e-m, release, code bytes)

| Image | Before P2.03 | After, `libc.a` | `libc-nofloat.a` | `libc-minimal-printf.a` |
|---|---:|---:|---:|---:|
| `hello-puts` | 2579 / 2279 | 2211 / 1931 | — | — |
| `hello-printf-int` | 8521 / 8261 | 8249 / 7981 | 5351 / 5159 | 4779 / 4271 |
| `hello-printf-float` | 11066 / 10246 | 10794 / 9966 | — | — |

- A `sscanf("%d %15s")` probe: 13332 → 3194 bytes on armv6-m and 10608 →
  2892 on armv7e-m with `libc-nofloat.a`. The `scanf` float path brings in
  strtod's whole engine.
- With the minimal flavour, armv7e-m and armv8-m.main link no libgcc at all
  (756 bytes before).
- armv6-m keeps 372 bytes of 32-bit division helpers; it has no divide
  instruction.

## Tests

- `test/unit/flavours`: one source, linked three times (against `libc.a`,
  `libc-nofloat.a` and `libc-minimal-printf.a`), and each run checks its
  flavour's behaviour:
  - the common conversions, flags, width, precision, `%b` and `wN`;
  - 64-bit lengths, refused only by the minimal flavour, and only where
    they are wider than `long`;
  - `%n` in `printf` and `scanf`;
  - floating point in `printf` and `scanf`, and that `strtod` still works.

  Each ELF fails when run with another flavour's expectations.
- `test/unit/stdio`:
  - a caller's buffer given up by a later `setvbuf` is not written;
  - a stream switched from unbuffered back to full buffering holds its
    output again;
  - the existing buffering, exit and file tests.
- `test/unit/stdio` `unbuffered-stdout`: `stdout` set to `_IONBF` writes
  through at once, before a direct write to its handle.
- Fault injection (x86_64 debug unless noted):
  - a kept caller buffer → `stdio` fails;
  - an unlink that leaves the list dangling → `stdio` hangs at exit;
  - the minimal octal base → `flavours-minimal` fails;
  - the `nofloat` stub reporting success → `flavours-nofloat` fails;
  - the minimal `scanf` store refusing nothing → `flavours-minimal` fails
    on armv6-m. On x86_64 nothing is wider than `long`, so nothing is
    refused there.

  One mutant is equivalent: `_IONBF` keeping the old buffer. The write and
  read paths test the unbuffered flag, not the buffer, so only 256 bytes
  stay idle.
- Reference programs `hello-printf-int-nofloat` and `hello-printf-int-minimal`
  join `test/size`. Their stack depths are measured against `libc.a`'s call
  graph, an upper bound.
