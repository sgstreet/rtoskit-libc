# Research: allocator fast paths

Step: P3.04b. Decision: PLAN D80.

## Starting point (P3.04a speed baseline, instructions per call)

| Case | armv6-m | armv7e-m | x86-64 |
|---|---:|---:|---:|
| `fixed/16` | 224 | 92.5 | 84 |
| `fixed/256` | 224 | 92.5 | 84 |
| `batch/fifo` | 228 | 96.6 | 85.1 |
| `batch/shuffled` | 247 | 111 | 101.8 |
| `batch/churn` | 256 | 118 | 110.4 |
| `realloc/grow` | 225 | 99.2 | 93.6 |

The default allocator (`nano`, D32) is first fit over one address-ordered
free list, merging on every free. In these workloads the list stays short:
- a freed block merges with its neighbours;
- the remainder of the region is usually the first chunk that fits.

Replacing the locks with no-ops (P3.04a) left about 46 instructions per
call on armv7e-m. That is the call, the size computation, the heap's
initialisation test, one or two list steps and the header writes. After
P3.04a's lock fast path, the lock pair costs about 46 instructions more.

## Sources reviewed

- **glibc**, *Malloc Internals*
  (<https://sourceware.org/glibc/wiki/MallocInternals>):
  - fast bins and the per-thread cache are exact-size LIFO lists of
    unmerged chunks;
  - they are consolidated into the normal bins when a large request
    arrives (`malloc_consolidate`).
- **D. Lea, *A Memory Allocator*** (<https://gee.cs.oswego.edu/dl/html/malloc.html>):
  dlmalloc's small bins, and the fragmentation cost of deferred
  coalescing.
- **M. Masmano et al., *TLSF: a New Dynamic Memory Allocator for Real-Time
  Systems*** (ECRTS 2004): two-level segregated fits, O(1) by size class.
  This is `LIBC_MALLOC=tlsf` here (D31).
- **musl mallocng**: size-class groups with slot bitmaps, about 40 KiB of
  code and metadata, beyond these targets.
- **Zephyr `sys_heap`**: log2 size buckets over a chunked heap, with
  coalescing on free.

## Measured (speed flavour, change from the baseline above)

1. **Twelve exact-size bins in front of the list.** Freed chunks of the 12
   smallest strides wait in LIFO lists, unmerged; an allocation pops the
   exact bin first. The bins are flushed into the list when first fit
   fails, so they never cause a failure.

   | Case | armv6-m | armv7e-m | x86-64 |
   |---|---|---|---|
   | `fixed/16` | −6 % | −12 % | −4 % |
   | `batch/shuffled` | −5 % | −8 % | −16 % |
   | `batch/churn` | +7 % | +11 % | −11 % |
   | `batch/fifo` | +5 % | +11 % | +4 % |
   | `fixed/256` | +3 % | +7 % | +13 % |
   | `calloc/256` | +7 % | +12 % | +21 % |
   | `realloc/grow` | +66 % | +117 % | +223 % |

   Chunks held in bins are not merged, so the free list fragments:
   - first fit walks more holes for the larger sizes;
   - `realloc` can no longer grow in place, so it moves and copies.
2. **The same, consolidating the bins before any larger request**, as
   glibc does. Worse everywhere except `fixed/16`: +17 % to +61 %, because
   the flush walks every bin, and mixed sizes flush constantly.
3. **Three bins (strides up to 24 bytes on 32-bit cores, 48 on x86-64),
   without consolidation.** `fixed/16` −6 % to −12 %, but every other case
   is 1–19 % slower, and `realloc/grow` +48 % to +134 %:
   - one cached chunk changes where the next small buffer lands, so it can
     no longer grow in place;
   - every `free` pays for the size test.
4. **TLSF instead of nano** (`LIBC_MALLOC=tlsf`, speed flavour): about 200
   instructions a call on armv7e-m and x86-64, 70–170 % slower in every
   case. Its time bound is constant, but larger than nano's walk over a
   short list. It is also 700 bytes larger.
5. **nano at `-O2`:** x86-64 1–3 % faster; armv7e-m within noise; Armv6-M
   about 1 % slower.

## Decision

- **Allocator:** no change. With short free lists, nano's first fit is
  already a few list steps. Deferred coalescing loses more on mixed sizes,
  through fragmentation and lost in-place growth, than it saves on one
  size. A class-segregated allocator (TLSF) has a larger constant cost.
- **Locking** was the real fast path and was taken in P3.04a: malloc + free
  107.5 → 92.5 instructions on armv7e-m.
- **When to use TLSF:** for long-lived heaps with many free chunks,
  `LIBC_MALLOC=tlsf` stays the choice for bounded time.

## Fragmentation

The allocator is unchanged, so there is no fragmentation regression to
check. The experiments above are the evidence against deferred coalescing
here: one pinned small chunk was enough to break in-place `realloc` growth
in the benchmark.
