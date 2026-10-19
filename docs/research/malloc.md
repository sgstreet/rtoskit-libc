# Research: the memory allocator

Step: P1.04. Decisions: PLAN D11 (libc owns `malloc`), D31, D32 (two allocators,
selected at build time).

## Sources reviewed

- M. Masmano, I. Ripoll, A. Crespo, J. Real, *TLSF: a new dynamic memory
  allocator for real-time systems*, ECRTS 2004, and the journal version
  *Implementation of a constant-time dynamic storage allocator*, Software:
  Practice and Experience 38(10), 2008
  (<https://www.cs.york.ac.uk/rts/publications/Masmano2008.html>). The paper
  PDFs on gii.upv.es did not resolve from this session; the design below
  rests on the published summaries and the reference implementation's
  documentation.
- Matthew Conte's TLSF implementation, `tlsf.c` design comments
  (<https://github.com/mattconte/tlsf>): boundary-tag layout where the
  previous-block pointer lives in the previous free block's tail, two flag
  bits in the size word, small blocks in first-level list 0, second-level
  log2 of 4 or 5, allocation sizes rounded up to the next list class. It
  aligns to the word size only (4 bytes on 32-bit), which is less than ISO C
  needs on Arm EABI (`max_align_t` is 8).
- The rlsf documentation (<https://docs.rs/rlsf/>): `SLLEN` trades free-list
  count against internal fragmentation; no small-object fast path; a pool
  must be locked as a whole.
- umm_malloc README (<https://github.com/rhempel/umm_malloc>): 8-byte
  blocks addressed by 15-bit indices (heap ≤ 256 KiB), best fit by default
  over a doubly linked free list (O(free blocks)), about 4 bytes of overhead
  per allocation, optional integrity and poison checks.
- FreeRTOS `portable/MemMang/heap_5.c`
  (<https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/MemMang/heap_5.c>):
  first fit over an address-ordered free list with coalescing of neighbours
  (O(free blocks)), several non-contiguous regions defined once at start-up
  (`vPortDefineHeapRegions`), a malloc-failed hook.
- musl mallocng `src/malloc/mallocng/meta.h` (kraj/musl mirror): size-class
  slab groups with out-of-band metadata, hardened against corruption; it
  depends on `mmap`/`brk` and page granularity, which an MCU heap does not
  have. musl oldmalloc and dlmalloc are binned best-fit designs with
  `sbrk`/`mmap` growth; their bins resemble TLSF lists but search more
  than one bin, so the bound is not O(1).
- picolibc's allocator, `libc/stdlib/{malloc,free,realloc,memalign}.c` and
  `local-malloc.h` (<https://github.com/picolibc/picolibc/tree/main/libc/stdlib>,
  commit 0858f49, fetched with a sparse clone after the first raw fetch
  failed): a one-word size header, one singly linked free list kept in address
  order, first fit with the remainder split off, merging with both neighbours
  on free, `realloc` that grows into a free successor or the top of the `sbrk`
  heap, `memalign` that splits off the front and back pieces, and optional
  power-of-two buckets for small sizes (`__MALLOC_SMALL_BUCKET`). The heap
  grows with `sbrk`.
- The standard text: C23 7.24.3 (memory management functions), 7.26.2.6/7
  (`strdup`, `strndup`).

## Comparison

| Allocator | malloc/free bound | Fragmentation | Overhead per block | Heap limits | Fit for an RTOS libc |
|---|---|---|---|---|---|
| TLSF | O(1) both | good fit, close to best fit; bounded by the second-level granularity | 1 word (size + 2 flags) when used | any size, any number of pools | yes: deterministic and multi-region |
| umm_malloc | O(free blocks) | best fit | 4 bytes | ≤ 256 KiB per heap | small heaps only |
| FreeRTOS heap_4/5 | O(free blocks) | first fit + coalescing | 2 words | none; heap_5 is multi-region | not deterministic |
| dlmalloc / musl oldmalloc | O(bins), with `sbrk` growth | best fit | 2 words | needs `sbrk` | heavier, not bounded |
| musl mallocng | amortised | slab | out-of-band | needs `mmap` | no |
| newlib/picolibc nano | O(free blocks) | first fit | 1–2 words | needs `sbrk` | small, not deterministic |

**Chosen: two allocators, selected at build time (D32).** TLSF is twice the
size of a first-fit allocator (measured below), which is too much as the
default for small parts. So:

- **`nano` (default):** first fit over one address-ordered free list, written
  from the design described for picolibc's allocator, but on the fixed heap
  regions of our seam instead of `sbrk`.
- **`tlsf`:** O(1) `malloc` and `free` with best-fit-like fragmentation, for
  systems that need bounded allocation time (`LIBC_MALLOC=tlsf`).

Both support several fixed regions (RP2350 SRAM banks, external PSRAM), align
payloads to two words, cost one word per used block, and share `calloc`,
`free_sized` and `free_aligned_sized`.

### Measured code size

Thumb-2, `-Os`, GCC 14.3 (Arm GNU Toolchain 14.3.rel1), text bytes. picolibc
was compiled from the sources above with its default options (no small
buckets); its numbers do not include `sbrk`, ours include region set-up.

| Allocator | `malloc` + `free` | + `realloc` | + `aligned_alloc` | + `calloc` | Total | Checker (tests only) | Static RAM |
|---|---|---|---|---|---|---|---|
| nano, armv6-m | 358 | 184 | 196 | 100 | 838 | 208 | 12 + 12 per region (in the heap) |
| nano, armv7e-m | 346 | 180 | 196 | 52 | 774 | 192 | same |
| tlsf, armv6-m | 988 | 216 | 228 | 100 | 1532 | 440 | 5 + control block in the heap |
| tlsf, armv7e-m | 960 | 228 | 240 | 52 | 1480 | 392 | same |
| picolibc, armv6-m | 412 | 224 | 154 (`memalign`) | — | 790 | — | 12 |
| picolibc, armv7e-m | 412 | 216 | 154 (`memalign`) | — | 782 | — | 12 |

On armv6-m, TLSF also pulls in libgcc's count-leading/trailing-zeros helpers.
The 4.2 KB first quoted for TLSF was x86_64 code including the checker.

### nano design

- Same chunk header as TLSF: the word before the payload holds the stride to
  the next payload, payloads are aligned to `2 * sizeof(size_t)`, and the
  minimum stride is one alignment unit (header plus the free-list link).
- `malloc`: first fit; a remainder of at least one minimum chunk replaces the
  chunk in the list, so the list stays in address order with no re-insertion.
- `free`: walk to the insertion point, then merge with the successor and the
  predecessor when they touch. O(free chunks).
- `realloc`: shrink in place; grow into the chunk just after it if that chunk
  is free and big enough; otherwise allocate, copy, free.
- `aligned_alloc`: as for TLSF.
- A three-word record at the start of each region (next, first, end) exists
  only so `__libc_malloc_check()` can walk the region.
- Trade-off against TLSF: allocation and free time grow with the number of
  free chunks, and first fit fragments more under some patterns.

## TLSF design

- **Alignment:** every payload is aligned to `ALIGN = 2 * sizeof(size_t)` (8 on
  Arm, 16 on x86_64), asserted to be at least `alignof(max_align_t)`. On x86_64
  that also meets GCC's assumption that `malloc` returns 16-byte aligned memory.
- **Block layout** (W = `sizeof(size_t)`): a block is named by its payload
  address `p`. The word at `p - W` holds the *stride*, the distance to the next
  block's payload (a multiple of `ALIGN`), with bit 0 = this block is free and
  bit 1 = the previous block is free. A used block can use `stride - W` bytes.
  A free block keeps its free-list links in its first two payload words and a
  back pointer to itself in the word at `p + stride - 2W`, which is the word
  just before the next block's size word. That word belongs to the payload
  while the block is in use, so a used block costs one word. The minimum
  stride is `4W` rounded up to `ALIGN`.
- **Mapping:** with `SL_LOG2 = 4` (16 second-level lists) and
  `SMALL = 16 * ALIGN`, strides below `SMALL` go to first-level list 0,
  second-level `stride / ALIGN`. Larger strides with `f = floor(log2(stride))`
  go to first-level `f - log2(SMALL) + 1`, second-level
  `(stride >> (f - 4)) - 16`. A request is rounded up to the start of the next
  class, so the head of any non-empty list at or above that class fits
  (good fit, no list scanning).
- **Search:** a bitmap of non-empty first-level lists and one 32-bit bitmap per
  first level; `stdc_trailing_zeros` finds the first usable list.
- **Control block:** carved from the start of the first region. Its list array
  is sized for the largest region (first-level count from the region's
  size), so a 256 KiB heap spends about 0.8 KiB on lists and a 64 MiB test heap
  about 1.3 KiB.
- **Regions:** fetched once, under the lock, at the first allocation, through
  `__rtos_heap_region(index, &base, &size)` until it returns false (up to 8).
  Each region ends in a zero-stride used sentinel, so blocks never merge across
  regions.
- **Free:** immediate coalescing with the next block (if free) and the previous
  block (bit 1 plus its back pointer), then insertion in its list.
- **realloc:** shrink in place, splitting off the tail when it is at least a
  minimum block; grow in place by absorbing a free next block; otherwise
  allocate, copy, free.
- **aligned_alloc:** for alignments above `ALIGN`, take a block of
  `size + alignment + minimum stride`, place the payload at the first aligned
  address that leaves either no gap or a gap of at least one minimum block,
  free the gap and trim the tail. Alignments that are not powers of two return
  a null pointer (C23 7.24.3.1).
- **Locking:** one seam lock (`__rtos_malloc_lock`) around every public entry.
- **Checking:** `__libc_malloc_check()` walks every region and verifies strides,
  flag consistency, the no-adjacent-free-blocks invariant, back pointers, and
  that every free block sits in the list its stride maps to, with bitmaps that
  match the lists. Used by the unit tests; not called by the library.

## Seam

- **Heap:** `bool __rtos_heap_region(unsigned int index, void **base, size_t *size)`
  rather than `__rtos_sbrk`: fixed regions match MCU linker scripts and
  multi-bank memory, need no growth protocol, and keep allocation
  deterministic. A port with growable memory reserves a region up front (the
  Linux port maps 64 MiB with `MAP_NORESERVE`, so untouched pages cost nothing).
- **Locks:** opaque `struct __rtos_lock` defined by the port; libc only holds
  pointers and names statically allocated locks the port must provide
  (`__rtos_malloc_lock` now; stdio, `atexit` and environment locks join with
  their steps). `__rtos_lock_acquire`/`__rtos_lock_release` are recursive.
  `__rtos_lock_create`/`__rtos_lock_destroy` serve dynamically created locks
  (per-`FILE`, P1.09). Because libc never sees the lock's size, one `libc.a`
  serves every port (D4, D15).
- `port/none`: single-threaded no-ops. `port/linux` until P1.12: a depth counter
  that aborts on a release without an acquire, so unbalanced locking shows up
  in tests.

## Implementation-defined choices (C23 J.3.12)

- `malloc(0)`, `calloc(0, n)`, `aligned_alloc(a, 0)` return a unique pointer to
  a minimum block (libc-test `regression/malloc-0` expects that).
- `realloc(p, 0)` is undefined behaviour in C23; ours returns a minimum block
  like `malloc(0)` (shrinking `p` in place), never freeing `p` and returning a
  null pointer.
- `free_sized` and `free_aligned_sized` free the block. A wrong size is
  undefined behaviour and is not diagnosed.
- Exhaustion and overflow (`calloc`, huge requests) return a null pointer with
  `errno = ENOMEM` (12, D20).

## Size

See "Measured code size" above. Phase 2 records sizes per arch in
`docs/size/`.

## Edge cases and test vectors (test/unit/malloc, run once per allocator)

- Alignment of every result to `ALIGN`; `aligned_alloc` with 1..4096 and a
  non-power-of-two alignment.
- `malloc(0)` unique pointers; `calloc` zeroing and `n * size` overflow →
  null + `ENOMEM`; `malloc(SIZE_MAX)` and `malloc(SIZE_MAX - small)` → null,
  no overflow in the stride computation.
- Exhaustion: allocate until failure, check `ENOMEM`, free everything, check
  that one allocation of nearly the whole heap succeeds again (coalescing).
- `realloc`: null pointer acts as `malloc`; shrink keeps contents; grow in
  place and by moving keeps contents.
- Randomised fuzz: thousands of mixed `malloc`/`calloc`/`realloc`/
  `aligned_alloc`/`free`/`free_sized` operations with pattern-filled payloads,
  verifying contents and `__libc_malloc_check()` after every step.
- `strdup`/`strndup`: copies, the `n` bound, an unterminated source within `n`.
- Leaks: the largest single allocation on the fresh heap (found by
  bisection) must succeed again whenever everything has been freed.
