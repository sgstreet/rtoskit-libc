# Research: benchmarks and how to measure them

Step: P3.01. Decision: PLAN D72.

## Sources reviewed

- **glibc `benchtests`** (<https://github.com/bminor/glibc/tree/master/benchtests>,
  `benchtests/README`, `bench-skeleton.c`):
  - Each math function has an input file (`##args`, `##ret`, one input per
    line, `##name:` to split the input domains, which take different
    precision paths, into separate results).
  - Timing uses the architecture's high-precision counter (`hp_timing`, `rdtsc`
    or `rdtscp` on x86), or `CLOCK_MONOTONIC`. The default is 10 seconds per
    function.
  - Each function reports **reciprocal throughput** (independent calls in a
    loop) and **latency** (each result feeds the next argument).
  - `workload-<name>` traces replay recorded arguments.
  - The string benchmarks (`bench-memcpy` and friends) write JSON over size ×
    alignment sweeps, and `compare_strings.py` tabulates them.
- **Arm optimized-routines** `string/bench/{memcpy,memset,strlen}.c`
  (<https://github.com/ARM-software/optimized-routines/tree/master/string/bench>):
  - A *random* workload draws sizes and alignments from frequency tables
    measured on SPEC2017: most `memcpy` calls are 32 bytes or shorter, and the
    sources are mostly 8- or 16-byte aligned.
  - A *medium* workload covers aligned and misaligned power-of-two sizes from
    8 to 512 bytes, and a *large* one covers bigger sizes.
  - The output is bytes per nanosecond per implementation, side by side.
- **Bionic** `benchmarks/string_benchmark.cpp`, `bionic_benchmarks.cpp`
  (<https://android.googlesource.com/platform/bionic/+/7447a1cee/benchmarks/bionic_benchmarks.cpp>):
  - Built on Google Benchmark, with one- and two-buffer alignment arguments
    (`AT_ALIGNED_ONEBUF`, `AT_ALIGNED_TWOBUF`, manual `ALIGN1`/`ALIGN2`).
  - Size classes are SMALL (1–256), MEDIUM (512 B–128 KB) and LARGE
    (256 KB–2 MB).
  - Separate suites cover stdio, stdlib, `pthread` mutexes uncontended and
    contended, and malloc.
- **picolibc** and **newlib**: neither ships a benchmark suite. Picolibc's
  published comparisons are about size
  (<https://www.keithp.com/documents/lca2020-picolibc.pdf>,
  <https://developer.espressif.com/blog/2026/04/esp-idf-6-default-libc-picolibc/>).
  Arm's LLVM-libc talk (<https://llvm.org/devmtg/2025-10/slides/lightning_talks/huynh.pdf>)
  compares math and Dhrystone against picolibc on a Cortex-M55, as relative
  figures only.
- **QEMU TCG plugins**: `docs/devel/tcg-plugins.rst` and
  `tests/plugin/insn.c` in qemu 8.2.2.
  - At translation time a plugin can attach to every guest instruction an
    inline operation (`qemu_plugin_register_vcpu_insn_exec_inline`,
    `QEMU_PLUGIN_INLINE_ADD_U64`) and a callback
    (`qemu_plugin_register_vcpu_insn_exec_cb`).
  - `qemu_plugin_outs` writes to the log, so `-d plugin` is needed to see
    it.
  - 8.2 has no register reads and no per-vCPU scoreboards (both arrived in
    9.0). A plugin therefore sees *where* the guest is, not its data.
- **QEMU without plugins:** `-one-insn-per-tb -d exec,nochain` logs one
  `Trace` line per executed instruction. It needs no build, but it ran at
  about 230 k instructions/s here, against more than 100 M/s with an inline
  counter.
- **Hardware:** the DWT `CYCCNT` of a Cortex-M3/M4/M7/M33 (Armv7-M/v8-M ARM
  C1.8; Armv6-M has no DWT cycle counter). It needs a board and a port
  (`port/none`). No board is attached to this environment (Q5).

## Trade-offs

**Instruction counts versus time:**

- An instruction count from the emulator is exact and repeatable to the
  instruction. It is independent of the host, and it costs little to run:
  the whole suite takes under 4 s for six boards.
- It ignores the pipeline: the cost differences between `LDM` and `LDR`,
  between a taken branch and an IT block, wait states, and the FPU's latency
  (a `VDIV.F32` costs 14 cycles on a Cortex-M4 and counts as 1).
- It is the right gate for an algorithmic change, and only a proxy for
  micro-architectural tuning. P3.05's Cortex-M7 work has to say so, or use a
  board (Q5).
- Timing on the x86_64 host:
  - The measurement is real, but this machine is a shared VM.
  - With repeated runs of the same binary, a per-case **median** of 9 moved
    by 0.6–1.6× (5th to 95th percentile).
  - The **minimum** of 15 still moved by 0.62–1.53×, because the host clock
    varies between runs. One case moved from 320 to 474 cycles with an
    identical instruction count.
  - So cycles are reported for context and never gated.

**Counting a region:**

- **Differential runs.** Run once with N iterations and once with 0, and
  subtract the whole-program counts. This needs no plugin logic, but two
  runs per case.
- **Marker functions** (chosen). The plugin watches two addresses, of the
  out-of-line `bench_roi_begin` and `bench_roi_end`. The runner reads them
  with `nm` and passes them as plugin arguments, and the plugin prints one
  count per region. One run covers every case of a program. The markers'
  own instructions are measured by an empty first region and subtracted.

**Workloads:**

- glibc and Bionic sweep size × alignment. optimized-routines adds a
  realistic random mix.
- Copying the SPEC2017 frequency tables would import someone else's data,
  so the mix here comes from its own fixed generator.
- The sweep stops at 1024 bytes. Embedded copies are short, and under qemu
  the count of larger sizes is a linear extrapolation that adds nothing.

## Method (P3.01)

**Harness:** `test/bench/harness`.

- Objects, not an archive, linked by every program, with a cycle counter
  per `ARCH_FAMILY`:
  - x86_64: `lfence; rdtsc; lfence`.
  - Arm: nanoseconds from `timespec_get(TIME_MONOTONIC)`, because qemu-user
    gives an M-profile program no cycle counter.
- `bench_run(name, units, body, context)` runs a body.
  - **count mode** (the default): the body runs once to warm up, outside the
    region, so one-time costs (the first heap growth, stream buffers) stay
    out. It then runs once between the markers.
  - **time mode** (`argv[1] == "time"`): the iteration count doubles until a
    run lasts 10^6 cycles, and the fastest of 15 runs is kept.
  - Either way it prints `bench <name> <operations> <cycles>`.
- Programs compile with `-fno-builtin -fno-tree-loop-distribute-patterns`:
  - every library call stays a call;
  - the harness's own copy loops do not become `memcpy`;
  - results go to `volatile bench_sink`, and the pointer arguments pass
    through an empty `asm` barrier (`bench_clobber`), so nothing is hoisted
    or discarded.

**Programs:** one per area under `test/bench/`. They are built for every
board and build type like `test/size`, and run only by the runner.

| Program | Cases |
|---|---|
| `bench-string` | `memcpy`, `memmove` (destination 8 below and 8 above the source, both overlapping), `memset`, `memcmp`, `memchr`, `strlen`, `strchr`, `strcmp`, `strcpy`; 15 sizes 0–1024 × 4 offset pairs (`0:0 1:1 0:1 3:2`, or `0 1 2 3` for one pointer); plus 256-call mixes of `memcpy`, `memset` and `strlen` with sizes from a fixed LCG (mostly under 16, tail to 255) |
| `bench-stdio` | `snprintf` per conversion (`%d %u %x %08x %lld %s %-20s %c %p`, a literal, `%f %.3f %e %g %.17g %a`), a log-line mix, the same through `fprintf` to `/dev/null` (a fully buffered stream), `fputs`, and `sscanf` of `%d %x %lf` and a mix |
| `bench-strto` | `strtol`/`strtoul`/`strtoll`/`strtoull` on short, long, negative and hex input, `atoi`; `strtod` on the classes that take different converter paths (integer, short, 17 digits, 50 digits, exponents ±300, the smallest normal, the smallest subnormal, hex, `1e-6`); `strtof` |
| `bench-malloc`, `bench-malloc-tiny` | malloc+free of 16, 256 and 4096 bytes; 64 blocks of 8–263 bytes freed FIFO, LIFO and shuffled, and a churn that refills holes with other sizes; a buffer grown from 16 to 4096 bytes by `realloc`; `calloc`. The same objects linked with `libc.a` (`LIBC_MALLOC`, `nano` by default) and with `libc-tinymalloc.a` (D54) |
| `bench-qsort` | `qsort` of 16, 256 and 1024 random ints, and of 1024 sorted, reversed and equal ones; 256 strings through `strcmp`; 64 `bsearch` lookups in 1024. The input is restored by a plain loop, so `memcpy` does not leak into these figures |
| `bench-math` | 31 functions (`sqrt cbrt hypot exp exp2 expm1 log log2 log10 log1p pow sin cos tan asin acos atan atan2 sinh cosh tanh asinh acosh atanh erf erfc lgamma tgamma fmod floor round`), float and double, 16 arguments over a typical domain, as throughput and as latency (`f(x[k] + y * 0)`) |
| `bench-threads` | `mtx_lock`+`mtx_unlock` for plain, recursive and timed mutexes, `trylock`, a nested recursive level, `call_once` once done, `tss_get` |

**Measurement:** `tools/scripts/bench-runner run`.

- Every board runs under a qemu built with plugins:
  - `make setup-bench` builds qemu 8.2.2 `arm-linux-user` and
    `x86_64-linux-user` with `--enable-plugins` into `local/qemu-8.2.2`,
    because Ubuntu's `qemu-user` has no `-plugin`. It takes about 2 minutes
    and needs ninja, python3 and glib.
  - Next to it goes `tools/qemu-plugins/bench-roi.c`.
- The runner pairs the `bench` lines with the plugin's `bench-roi` lines.
  The case figure is (count − calibration) / operations, in instructions
  per operation.
- On x86_64 it also runs the program natively in time mode, pinned with
  `taskset -c 0`, and records cycles per operation.
- The result is `build/<board>/release/test/bench/results.json`.

**Baseline and gate:**

- `report` copies the results to `docs/perf/baseline-<arch>.json`.
- `diff` fails on any case whose count grew by more than 1 % and more than
  2 instructions. Counts do not move between runs (checked: two runs on
  every board are byte-identical), so the margin only absorbs harness
  noise, never measurement noise.
- `markdown` prints the tables in `docs/perf/<arch>.md`. With `--previous
  <revision>` it shows the change against that revision's baseline.
- `make bench` builds, runs and diffs the release build of every board.
  `BENCH_UPDATE=1` records instead.

## Edge cases

- **Thumb symbols** have bit 0 set. The runner clears it on Arm only,
  before passing the address. Clearing it in the plugin broke x86_64,
  where `bench_roi_end` sat at an odd address one byte after the `ret` of
  `bench_roi_begin`, and every region counted 1.
- **Markers must stay calls.** `[[gnu::noipa]]` with an `asm` memory
  barrier stops GCC from inlining them, cloning them or proving them empty.
  The begin callback fires before the marker's first instruction executes,
  and the end callback before the end marker's.
- **Debug builds** compile the benchmarks too (`-Werror` coverage), but
  `make bench` measures release only.
- **Single thread:** the plugin's counter is a plain global. A second vCPU
  thread would race it. `bench-threads` measures uncontended operations, so
  it never starts a thread.
- **Determinism:** the heap, the stream buffers and the arguments are the
  same on every run (fixed LCG seeds, no time or address input), so the
  counts are byte-identical between runs.

## Test vectors and validation

- **Plugin against an independent count:** for `bench-threads` and
  `bench-strto` on armv7e-m, the exec trace of the system qemu
  (`-one-insn-per-tb -d exec,nochain`), counted between the marker
  addresses, gives the same number for every one of the 30 regions as the
  plugin. The calibration region is 6 instructions on Thumb-2: two
  `bx lr`, two calls and the body call.
- **Repeatability:** two complete runs produce identical `results.json`
  files on the five Arm boards, and identical instruction counts on x86_64.
- **The gate:** the diff was tested by fault injection. Raising one
  baseline count makes `diff` exit 1 and name the case. Lowering it reports
  the case as faster and exits 0.
- **Sanity:**
  - `memcpy` of 1024 aligned bytes costs about 4.0 instructions per byte on
    armv7e-m (the Thumb-1 kernel, D70) and 1.4 on x86_64 (the generic C,
    word at a time).
  - `sinf` costs 685 instructions on armv7e-m (float-float, D60).
  - `sin` costs 6800 on armv7e-m (soft double) and 308 on x86_64.
  - `strtol("42")` costs 949: the first finding for P3.03.
