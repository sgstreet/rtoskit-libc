# Research: size of the allocator, stdlib, string and time code

Step: P2.04. Decision: PLAN D54. Measurements: `tools/scripts/size-report`
(D51) and probe programs linked like `test/size`, release build.

## Sources reviewed

- B. Kernighan and D. Ritchie, *The C Programming Language*, 2nd ed., 8.7: a
  circular explicit free list with headers.
- R. Bryant and D. O'Hallaron, *Computer Systems: A Programmer's Perspective*,
  9.9:
  - implicit free lists (each header holds size plus an allocated bit; the
    heap is walked);
  - immediate versus deferred coalescing;
  - sentinel "epilogue" blocks that end the walk.
- FreeRTOS `heap_1`/`heap_2`/`heap_4` (design descriptions only):
  - allocate-only;
  - best fit without merging;
  - first fit with merging.
- umm_malloc and newlib's nano-mallocr (structure only): explicit
  address-ordered free lists, one header word.
- GCC's ARM EABI runtime (`__aeabi_uldivmod`, `__aeabi_ldivmod`, `__udivmoddi4`
  in libgcc): a 64-bit division costs 300 bytes (armv6-m) to 900 bytes
  (armv7e-m, armv8-m.main) of library code. GCC does not turn a 64-bit
  division by a constant into a multiplication on 32-bit Arm.
- H. S. Warren, *Hacker's Delight*, 9-2 (long division by digits) and 9-3
  (division by a single-word divisor in parts).
- The standard: C23 7.24.3 (memory management), 7.26.6.3 (`strerror`), 7.4
  (`<ctype.h>`) and 7.29 (time).

## Allocator

| Option | Code (armv6-m, `malloc-heavy`'s allocator part) | Notes |
|---|---:|---|
| nano (D32): explicit address-ordered free list, merge on free | 658 | Default |
| Implicit list, multi-region, region table | 614 | First try: −44, not worth an archive |
| Implicit list, first region only, a used sentinel of stride 0 ends the heap; free clears a bit; allocation walks first fit merging free runs (deferred coalescing) | 574 | Chosen as the tiny allocator: −13 % (−11 % on armv7e-m), one word per chunk, the same alignment |
| Allocate-only (heap_1) | ~150 | Rejected: `free` must make the space available again (7.24.3.3), and the malloc tests (exhaustion and recovery) fail |

- The tiny allocator is a third variant, `libc/malloc/tiny`, built and
  tested like nano and tlsf. It is selectable as `LIBC_MALLOC=tiny`, and is
  always available as `libc-tinymalloc.a` (libc.a with its allocator
  replaced, D53's mechanism).
- Its `malloc` is O(chunks), and it uses only the port's first heap region.
  Both Linux and `port/none` offer one.
- nano itself is already close to minimal: one header word, two-word
  alignment (needed for `max_align_t`). It was left unchanged.

## 64-bit division

- Eleven members divided 64-bit values by small constants:
  - digits for printf, `strftime` and `asctime`;
  - `strtoint`'s overflow test, run on every digit;
  - `strtofloat`'s fast path and `decimal_set`;
  - the calendar (`secs_to_tm`, `tm_to_secs`, `days_from_civil`,
    `civil_from_days`, `is_leap`, the TZ rules).

  Each pulled libgcc's 64-bit division into every program that used it,
  and every `printf` image paid for it.
- `__libc_udivmod64(n, d, &r)`, for divisors up to 2^31:
  - the high word divides with a 32-bit division;
  - the low word's 32 bits are brought down one at a time (shift and
    subtract, Warren 9-2);
  - the remainder never exceeds the divisor, so nothing overflows;
  - on 64-bit targets the division is native (a `sizeof` test that folds).
- `__libc_floor_divmod64` builds floor division on it through `~n` (which
  is −n−1) and so needs no special case for `INT64_MIN`.
- `strtoint` now computes the cutoff and the cutoff digit once, not a
  division per digit.
- `lldiv` and `imaxdiv` keep native division: the caller asked for one.
- Cost: up to 32 iterations per 64-bit digit. Only values above 2^32 take
  that path in printf. Phase 3 can restore a faster variant for the speed
  flavour.

## strerror, ctype, qsort

- `strerror`:
  - Before: a `switch` over the sparse Linux numbers (0–131), which GCC
    compiled to a pointer table indexed by the number (about 530 bytes) plus the strings.
  - Now: one array of records (the number plus one, the message, a null
    byte), walked to the match: one byte of index per message.
  - 2395 → 1968 bytes.
  - A golden test pins every number to its message, and it passed against
    both versions.
- ctype:
  - Each classification function is 14–34 bytes of code (ranges and bit
    tests).
  - A 256-entry class table would cost 256 bytes of rodata plus about
    10 bytes per function, so a program would need most of the twelve
    functions before a table paid off.
  - The code stays, on every architecture.
- `qsort` is a 228-byte heapsort (no recursion, no extra memory, D33-era
  choice). Its word-sized swap is a speed path worth keeping for Phase 3.
  Unchanged.

## Time

- `strftime` (2.2 KB), the TZ parser (~1.3 KB) and the calendar were
  already table-light. Their largest avoidable cost was libgcc's 64-bit
  division. A localtime + strftime + mktime probe drops from 6344 to 5470
  bytes on armv7e-m (libgcc 912 → 0) and from 6904 to 6122 on armv6-m.
- `__libc_is_leap` now reduces the year modulo 400 once.
- `tm_to_secs` folds months in `int`.
- `%C` keeps C's truncation toward zero, through sign and magnitude.

## Tests

- `test/unit/div64`: `__libc_udivmod64` and `__libc_floor_divmod64` against
  native division for the library's divisors, 1, 2^31 − 1, 2^31, and 40 000
  random divisors, with edge and random dividends.
- `test/unit/malloc`: a third executable, `malloc-tiny`, runs the whole
  allocator test, including exhaustion, recovery and the fuzz run with the
  heap checker.
- `test/unit/string`: every errno number with its exact message.
- Existing tests: time (gmtime/localtime/mktime/strftime far from the
  epoch), strtol/strtoull boundaries, printf of 64-bit values, libc-test.
- `test/size/malloc-heavy-tiny`: `malloc-heavy` linked against
  `libc-tinymalloc.a`.
