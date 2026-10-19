# Research: size measurement and the comparison libraries

Step: P2.01. Decision: PLAN D51.

## Sources reviewed

- Bloaty McBloatface (<https://github.com/google/bloaty>,
  `doc/using.md`): breaks a binary or a `.a` down by sections, symbols,
  segments or compile units (the last needs DWARF), with VM and file sizes
  side by side and a diff mode (`bloaty new.elf -- old.elf`). Not packaged
  here; it needs a C++ build with protobuf and capstone.
- puncover (<https://pypi.org/project/puncover>): a web report of code size,
  static variables and stack use per function, from an ELF plus the target's
  `objdump`/`nm`. It reads GCC's `.su` files to annotate the functions but
  needs a browser session; nothing to commit.
- GCC `-fstack-usage` (the `.su` file: `file:line:col:function bytes
  static|dynamic|dynamic,bounded`) and `-fcallgraph-info=su` (GCC 10+, a VCG
  graph per unit with one node per function carrying its frame, one edge per
  call site, and an `__indirect_call` placeholder for calls through
  pointers). AdaCore's GNATstack consumes the same `.ci` files
  (<https://docs.adacore.com/live/wave/gnatstack/html/gnatstack_ug/Getting_Started_with_GNATstack.html>)
  and states the limits of a static worst case: cycles, dynamic frames,
  external (assembly or library) callees and indirect calls.
- avstack.pl (Daniel Beer) and Embedded Artistry's *Three GCC flags for
  analyzing memory usage*
  (<https://embeddedartistry.com/blog/2020/08/17/three-gcc-flags-for-analyzing-memory-usage/>):
  the call graph is rebuilt from `objdump` disassembly and joined with
  `.su` frames. This is fragile on Thumb (tail calls, veneers) and needless
  now that GCC emits the graph itself.
- NuttX `tools/stackusage.py`
  (<https://nuttx.apache.org/docs/latest/debugging/stackusage.html>): the
  frames come from DWARF `.debug_frame` CFA offsets instead of `.su`.
- GNU ld map files (`-Map`, `--cref`): every input section is listed with
  its address, size and source object (`libc.a(member.o)`) under the output
  section it went into. This is the only source that splits an image
  between the program, the C library, the OS layer and libgcc.
- The comparison libraries: picolibc
  (<https://github.com/picolibc/picolibc>; the printf variants selected at
  link time through `--defsym=vfprintf=__i_vfprintf` and the like) and
  newlib-nano (`nano.specs`: integer-only `printf` unless
  `-u _printf_float`). Published comparisons (Zephyr's picolibc patch for
  Cortex-M3, the 2026 ESP-IDF note, Keith Packard's LCA 2020 slides) all
  measure a `printf` program and report whole images. They use different
  OS layers, so their numbers do not transfer.

## Trade-offs

| Need | Options | Chosen |
|---|---|---|
| Per-member size | `size -A` per member; `objdump -h`; read the ELF directly | Read the archive and the ELF section headers directly in Python: no output parsing, and the classes come from the section flags (`SHF_ALLOC`, `SHF_EXECINSTR`, `SHF_WRITE`, `SHF_TLS`, `SHT_NOBITS`), not from names |
| Image split | bloaty `-d compileunits` (needs DWARF and a tool not available); map file | The map: each input section is attributed to the program, `libc`, the port (OS layer), libgcc or the linker (`*fill*`, startup files) |
| Stack | `.su` alone (frames only); disassembly call graphs; `-fcallgraph-info=su` | `-fstack-usage -fcallgraph-info=su` on every compile. The frames are recorded per function, and the worst depth of selected entry points is the longest path through the merged graph |
| Comparison libraries | The toolchain's bundled picolibc 1.8.12; the Ubuntu 24.04 packages | The Ubuntu packages, pinned by version and SHA-256: picolibc 1.8.6 and newlib 4.4.0.20231231 (nano). The bundled picolibc is built as slim LTO objects, and its archive does not link here: the plugin leaves `printf` and `__libc_init_array` unresolved |

## Method

- **Metrics** (phase 2 header): for each `libc.a` member, `text`, `rodata`,
  `data`, `bss`, `tdata` and `tbss`, plus `unwind` (`.ARM.exidx`,
  `.ARM.extab`, `.eh_frame`) so that P2.02 can see the unwind tables. The
  same classes are reported for each reference image, together with the
  split by origin.
- **Reference programs** (`test/size/`) are ordinary modules linked through
  `port/linux` with `--gc-sections`, built in both build types. The release
  build is the one measured. Inputs come from `argc` and results go to the
  exit status or the output, so nothing folds away. They use the compiler's
  builtins, as user code does (so `printf("x\n")` becomes `puts`).
  - `hello-puts`: `puts` only.
  - `hello-printf-int`: `printf` with `%d %u %x %s %c`.
  - `hello-printf-float`: `printf` with `%f %e %g`.
  - `malloc-heavy`: `malloc`, `calloc`, `realloc`, `free`, `strdup`, no
    stdio.
  - `math-mix`: double and float elementary functions, no stdio.
- **Stack depth.** Calls resolve first within the unit (static functions),
  then globally; when a name has several definitions (link-time replacement,
  D13) the largest frame counts.
  - Some callees have no graph: functions of objects without a `.ci`
    (assembly in the library or the port, found from the symbol table) and
    libgcc's helpers (`__aeabi_*`, `__clzdi2`, `__muldc3`, ...). They are
    leaf routines that push a few registers.
    - A depth that misses only their frames is marked `*`.
    - A path through `__indirect_call`, recursion, a plain `dynamic` frame
      (alloca, VLA) or an unknown callee makes the depth a lower bound,
      marked `+`.
    - `dynamic,bounded` frames have a known size and count as exact.
    - The report names every reason.
  - Cycles are cut and reported. The library's recursion is bounded (for
    example `qsort` recurses only into the smaller partition).
  - The depth of each reference program's `main` is reported too.
- **Diff.** `size-report diff` recomputes the figures from the build tree
  and compares them with `docs/size/baseline-<arch>.json`. It prints the
  totals, the largest growths and reductions by member, image and stack
  depth, and exits with 1 if anything grew. Each step that keeps a growth
  justifies it in its notes (phase 2 DoD).
- **Comparison.**
  - `size-report compare` downloads the two packages into
    `tmp/size-compare/`, checks their SHA-256 (pinned in the script), and
    builds the same reference sources for the five Arm architectures with the
    same compiler and flags (`-Os -ffunction-sections -fdata-sections`,
    `--gc-sections`). The mode is `-std=gnu23`, because picolibc 1.8.6
    predates C23 and declares `strdup` only outside strict ISO mode.
  - picolibc links with `--oslib=semihost` and its own crt0 and linker
    script. newlib-nano links with `libnosys`, its crt0 and GCC's
    crti/crtbegin.
  - The fair column is the `libc` share of each image from the map, since
    the OS layers differ: Linux system calls through `port/linux`,
    semihosting, and stubs.
  - Each library keeps its default `printf`: picolibc's is the double
    version, newlib-nano's is integer-only. `hello-printf-float` adds
    `-u _printf_float` for newlib-nano so that it formats floats at all.
  - The comparison is for context only and is not a gate.

## Edge cases

- Merged input sections: duplicate constants (`.rodata.cstN`) and the CIEs in
  `.eh_frame` (x86_64) are merged by the linker.
  - The map gives the whole merge's size to the first input section and
    each input's own size on a `(size before relaxing)` line, which the tool
    uses.
  - An output section whose inputs then add up to more than its size has
    its shares scaled down to fit.
- x86_64's `.note.gnu.property` (32 bytes per object) is not counted:
  objects carry one note each, and an image carries one merged note.
- The host compiler of the x86_64 board assumes glibc and fuses `sin` and
  `cos` into `sincos`, which C does not reserve. `math-mix` failed to link
  until the board compiled with `-muclibc`, GCC's name for a Linux C library
  without `sincos`. `-fno-builtin-sincos` does not stop the fusion, which
  depends on the target's C library hook.
- The per-object `rm` list in `project.mk`'s `clean` grew too long for
  `libc/math` once `.su` and `.ci` were listed per object, and `clean` failed
  silently. They are removed by directory wildcard instead.

- `armv6-m` and `armv6s-m` produce the same `libc.a`, and both are
  recorded so that the files stay one per architecture.
- Sections that the map lists with size 0, and the discarded input
  sections, are skipped. `COMMON` symbols count as `bss`.
- A member name can occur twice in an archive in principle (two modules
  with the same file name). The report keys a duplicate as `name#2`. None
  exist today: 743 unique members on `armv6-m`.
- x86_64 is measured for reference only (phase 2 header). Its images
  contain the Linux port's x86_64 startup and no comparison is made.

## Test vectors

- The trial links on `thumb/v6-m/nofp`:
  - picolibc 1.8.6 with `printf("%d %s %x\n")` is 7644 bytes of text;
  - newlib-nano is 7556 bytes of text.
- The tool's own checks:
  - The per-member sums equal `size` over `libc.a` on every Arm board,
    with text + rodata + unwind against `size`'s text column. For example
    armv6-m gives 121409 / 124 / 1068, the Phase 1 table in
    `docs/size/phase-1.md`.
  - On x86_64 they differ only by the excluded notes.
  - The image figures equal `size` of each ELF on Arm.
  - The attributed shares add up to the image figures on every board.
    What the map does not name (startup files, alignment fill) is at most
    62 bytes of "other".
  - `diff` against an unchanged tree reports no change and exits 0.
  - Fault injection: a 200-byte local array added to `abs` on armv7e-m was
    reported as `abs.o` 8 → 26 (+18) and the frame `abs.o:abs` 0 → 200,
    with exit status 1. After the revert, the diff exited 0.
