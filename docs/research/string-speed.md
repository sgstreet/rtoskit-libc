# Research: the speed flavour and the speed string kernels

Steps: P3.02a (Thumb-2 `memcpy`/`memmove`/`memset`); later P3.02b–d.
Decision: PLAN D73.

## Sources reviewed

- **Armv7-M ARM** (DDI 0403E) A3.2 *Alignment support*: on Armv7-M,
  - LDR, LDRT, LDRH, LDRSH, STR, STRT and STRH may be unaligned;
  - LDM, STM, LDRD, STRD, PUSH, POP, LDREX and STREX always fault
    (UsageFault) on an unaligned address;
  - `CCR.UNALIGN_TRP` makes the first group fault too;
  - an unaligned access to Device or Strongly-ordered memory is
    UNPREDICTABLE.

  Armv8-M Mainline (DDI 0553) keeps the same rules.

  GCC follows them: `-munaligned-access` is the default for these
  architectures and defines `__ARM_FEATURE_UNALIGNED`, so compiled C already
  relies on them. Armv6-M has no unaligned access at all.
- **Cortex-M4 TRM** (DDI 0439) 3.3 *Instruction set summary*, timings:
  - LDR/STR take 2 cycles, and a run of independent loads pipelines to
    1 cycle each;
  - LDM/STM of N registers take 1 + N cycles;
  - an unaligned word access costs extra bus cycles (it is split);
  - a taken branch costs 1 + P (P = 1–3 refill cycles);
  - IT is folded or takes 1 cycle, and a skipped conditional instruction
    still takes its issue slot.

  Community measurements give 16 versus 10 cycles for four words by
  LDR/STR pairs versus one LDM/STM pair
  (<https://www.cse.scu.edu/~dlewis/book3/docs/Cortex-M4%20Instruction%20Timing.pdf>,
  <https://community.arm.com/support-forums/f/keil-forum/31706/hard-fault-at-misaligned-memcpy-memset/148299>).
- **Cortex-M7 TRM** (DDI 0489):
  - a dual-issue in-order pipeline;
  - a 64-bit AXI master and 64-bit TCM interfaces;
  - LDRD/STRD and LDM/STM transfer 64 bits per beat.

  User reports (<https://lists.rtems.org/pipermail/bugs/2026-May/059910.html>)
  confirm that LDRD and LDM fault on unaligned addresses there while
  LDR/STR do not.
- **LLVM libc** `libc/src/string/memory_utils/arm/inline_memcpy.h`
  (<https://llvm.googlesource.com/llvm-project/+/a37bb291fd9ca6c19a809196d074eaddae041353/libc/src/string/memory_utils/arm/inline_memcpy.h>):
  - two strategies, one for cores without unaligned access (M0/M0+/M23)
    and one for those with it (M3/M4/M7/M33);
  - the latter tests whether source and destination are misaligned
    relative to each other, and treats small sizes separately;
  - about 272 bytes as `memcpy`.
- **newlib** `libc/machine/arm/memcpy-armv7m.S` and the Linaro
  cortex-strings (<https://bazaar.launchpad.net/~linaro-toolchain-dev/cortex-strings/trunk/view/96/reference/newlib/memcpy.S>):
  - align the destination;
  - copy large aligned blocks with LDM/STM or LDRD/STRD;
  - when the source is misaligned, either rely on unaligned LDR or merge
    aligned words with shifts.
- **Arm optimized-routines** `string/arm/memcpy.S`: written for A-profile
  (NEON, LDRD), so not usable on M-profile. It confirms the shape: alignment
  head, an unrolled bulk loop, and a computed tail.

## The flavour mechanism (Q3, D12)

- **Measured first:** compiling the whole library with `-O2` on armv7e-m
  breaks the phase 2 budget's +10 % on every `test/size` image.
  - `hello-printf-int` grows 35 % (9677 against 7185 bytes);
  - `math-mix` 16 %;
  - even `hello-puts` 13 %.

  So the speed flavour cannot be a different optimisation level. It is
  `-Os` like release, with speed kernels and hot modules that opt into
  `LIBC_SPEED_CFLAGS` (`-O2`) on their own.
- **Candidates for how a build picks the flavour:**
  - A `LIBC_FLAVOUR` variable on the command line, like `LIBC_MALLOC`. Both
    flavours would share one build tree, so switching needs `make clean`,
    and `make check` would test only one.
  - A flavour archive in the release build, like `libc-nofloat.a` (D53).
    The speed members would be compiled twice, and libc-test would need a
    second link of every test against it.
  - **A build type `speed`** (chosen). `tools/makefiles/speed.mk` is
    included last by `common.mk`, as `release.mk` is. It sets
    `LIBC_FLAVOUR := speed`, and `BUILDS` gains `speed`. The flavour then
    gets its own tree (`build/<board>/speed/`) and install
    (`images/<board>/speed/`). `make check` runs every unit test and
    libc-test on it. `size-report --build-type speed budget --flavour speed`
    checks its budget, and `make bench` measures it. A Phase 4 multilib can
    point at either install.
- **Kernel directories:**
  - Each arch file now gives one list per flavour,
    `LIBC_STRING_KERNELS_size` and `LIBC_STRING_KERNELS_speed`, and
    `LIBC_STRING_KERNELS` expands the one `LIBC_FLAVOUR` names. The
    expansion is deferred, because the build-type file is read after the
    arch file.
  - A list may name several directories, and the first directory that has
    a function wins: each kernel directory's `subdir.mk` drops the `.S`
    files of the directories listed before it. So `thumb2 thumb1` takes
    `memcpy`/`memmove`/`memset` from `thumb2` and still `strlen`/`strcmp`
    from `thumb1` until P3.02b replaces them.

## Kernels (P3.02a)

All three are written from the ISA and the C23 text. They share one shape
and rely on unaligned LDR/STR/LDRH/STRH (never LDM/STM/LDRD/STRD on an
unaligned address).

**Zero bytes** return at once: `cbnz` past a `bx lr` in `memcpy`, and
`cbz` in `memset`.

**Below 32 bytes**, the *tail* alone runs:
- n is taken by its bits: 16 bytes as four word copies, 8 as two, 4, 2, 1;
- every access may be unaligned;
- each step sits behind a branch on the flags of one `LSLS` (`#28`: C is
  bit 4, N is bit 3; `#30`: bits 2 and 1; `#31`: bit 0).

The first version used IT blocks. Every conditional instruction there,
executed or not, costs an issue slot (and an instruction in the counts), so
a 1-byte copy paid for all the absent steps. Branches, the early return and a 32-byte threshold (in place of 16)
together took a 0-byte copy from 36 to 19 instructions and a 16-byte one
from 60 to 39. On a Cortex-M4, a skipped branch costs 1 cycle, against 3
for `ITT` plus two skipped instructions.

**From 32 bytes:**
- `push {r4-r9, lr}`;
- the destination is aligned with a byte, then a halfword, chosen by the
  bits of `-dst`;
- then 32 bytes per iteration:
  - **source aligned too**: `LDMIA`/`STMIA` of eight registers
    (r3–r9, lr): 4 instructions per 32 bytes; 9 + 9 cycles on an M4, and
    64-bit beats on an M7;
  - **source misaligned**: eight unaligned LDRs, an ADD, and one STMIA to
    the aligned destination (12 per 32 bytes). The alternative, aligned
    loads merged by `LSR`/`ORR` shifts, takes 3 instructions per word.
    Unaligned LDR costs an extra bus cycle each, but no ALU work.
- The remainder is n − 32 after the loop. Its low five bits are those of n,
  so the tail runs on them unchanged.

**`memmove`:**
- `(dst - src) >= n` (unsigned) means forward is safe, so it tail-calls
  `memcpy`. Every step of `memcpy` reads a block before writing it and moves
  upwards, so a destination below the source is never overwritten before it
  is read. Equal pointers return.
- Otherwise it mirrors `memcpy` from the ends, using `LDMDB`/`STMDB`, eight
  unaligned LDRs at negative offsets, and a pre-decremented tail. Every
  step moves down, which is what makes an overlap with the destination
  above the source safe.

**`memset`:**
- the byte is replicated by `UXTB` and a multiply by 0x01010101;
- the same tail with stores;
- from 32 bytes, one *unaligned word* goes to `dst` and the destination
  moves up by `(-dst) & 3`. The bytes stored twice hold the same value, so
  no byte/halfword branches are needed.
- then two `STMIA`s of four registers per 32 bytes, one more for a
  remaining 16, then the tail.

## The size budget

The first complete version was 200 + 218 + 132 bytes (`memcpy`, `memmove`,
`memset`, against 60 + 60 + 48 in the size flavour). That put
`malloc-heavy`, which links `memcpy` and `memset`, at +11.6 % over its
size flavour budget (2158 bytes against 2127) on armv7e-m, and
`malloc-heavy-tiny` at +10.4 % on armv8-m.main. Trimmed without touching
the block loops:

- **The 16- and 8-byte tail steps:** offsets plus one pointer update (the
  16-bit `LDR`/`STR` encodings), in place of post-indexed 32-bit pairs. The
  next step re-derives its flags with one `LSLS`.
- **The alignment head:** `k = (-dst) & 3` from `NEGS`/`LSLS`/`LSRS`, taken
  off n once, in place of two 32-bit `SUB`s.
- **The source-aligned test:** `LSLS #30` in place of `TST.W`.
- **`memset`:** the unaligned-word head, and `UXTB` and `MULS` in place of
  `AND` and two `ORR`s.

The kernels are now 180 + 218 + 108 bytes. Every `test/size` image of the
speed flavour is within the size flavour's budget + 10 %
(`size-report --build-type speed budget --flavour speed`). The tightest is
`malloc-heavy-tiny` on armv8-m.main, at 2034 bytes against 2039. The cost
is 2–3 instructions on copies with a 16- or 8-byte remainder.

**LDRD/STRD** (the phase file's wording) would also work on aligned words.
On an M4, `LDRD` takes 3 cycles per 8 bytes against 1 + N for LDM, and it
needs twice the instructions for the same block. The M7 does 64-bit beats
for both, so LDM/STM is used.

## Results (armv7e-m, speed against the phase 2 size flavour)

Instructions per call, including the harness's call and loop:

| Case | size | speed |
|---|---:|---:|
| `memcpy` 0 B | 33 | 19 |
| `memcpy` 16 B aligned | 53 | 42 |
| `memcpy` 31 B misaligned | 152 | 54 |
| `memcpy` 64 B aligned | 113 | 54 |
| `memcpy` 1024 B aligned | 1313 | 174 |
| `memcpy` 1024 B misaligned | 4124 | 441 |
| `memset` 64 B misaligned | 114 | 58 |
| `memset` 1024 B | 1054 | 177 |
| `mix/memcpy` (256 calls, mostly < 16 B) | 215 | 54 |
| `mix/memset` | 90 | 42 |

On each Thumb-2 core, 319 cases are faster and none is worse beyond the
gate. Six are 1–2 instructions worse: 1–4 byte `memcpy`/`memset` calls (the
early-return test and the branch-skipped steps) and `strcpy` of an empty
string. The full tables are in `docs/perf/<arch>.md`.

## Edge cases

- **n = 0**, including from `memmove` with distinct pointers, which reaches
  `memcpy`'s early return.
- **Equal pointers** in `memmove`.
- **Overlap by 1 to n − 1** in both directions.
- **Thresholds:**
  - n = 31 and n = 32 (tail only, or the first block);
  - n = 33–35 after alignment eats up to 3 bytes, which can leave fewer
    than 32 (the loop is skipped and the remainder bits still hold).
- **All 64 combinations of source and destination offsets modulo 8.**
- **The return value:** `ip` holds it, because r0 moves. In the block path
  `ip` is not among the pushed or loaded registers.
- **`CCR.UNALIGN_TRP`:** a system that sets it, or that runs `memcpy` on
  Device memory, must use the size flavour, which has no unaligned access.
  The same already holds for GCC's own code generation on these cores.

## Test vectors and validation

- `test/unit/string_kernels` runs on every flavour now that `speed` is in
  `BUILDS`:
  - every source and destination offset modulo 8;
  - every length up to 80 plus 127–129 and 255–257, which covers up to 8
    block iterations both aligned and misaligned;
  - guard bytes on both sides and the return values;
  - `memmove` in both directions over every overlap.
- Mutation check (armv7e-m speed), first version. Seven mutants fail the
  test:
  - `memcpy`: a wrong offset in the unaligned loop, a wrong count in the
    alignment head, the 8-byte tail step inverted;
  - `memmove`: the 8-byte tail step inverted, a short `LDMDB`;
  - `memset`: the wrong bit for the 16-byte step, the byte step inverted.

  The eighth survives: removing the equal-pointer early return in
  `memmove` copies the buffer onto itself, which is still correct.

  All six mutants of the trimmed code fail or crash the test: the `memset`
  head advance, an 8-byte step offset, a pattern register, the `memcpy`
  alignment count, a 16-byte step pointer update, and the source-alignment
  test.
- libc-test and every unit test pass on the speed build type of all six
  boards.
