# Research: a faster qsort for the speed flavour

Step: P3.07. Decision: PLAN D90 (answers Q17).

## Starting point (P3.06, instructions per element)

| Case | armv6-m | armv7e-m | x86-64 | picolibc (armv7e-m) | newlib-nano (armv7e-m) |
|---|---:|---:|---:|---:|---:|
| `qsort/random/1024` | 769 | 640 | 894 | 278 | 277 |
| `qsort/random/16` | 267 | 224 | 321 | | |
| `qsort/sorted/1024` | 813 | 677 | 948 | | |
| `qsort/equal/1024` | 139 | 114 | 157 | | |
| `qsort/strings/256` | 708 | 585 | 617 | | |

Every rtoskit flavour runs the heapsort of P1.05 (D33), chosen for its worst case, no recursion and no extra memory. It makes about 2n log2 n comparisons, and it moves elements far apart on every sift. A quicksort makes about 1.39 n log2 n comparisons on random input and swaps fewer, closer elements. picolibc and newlib both ship a BSD-derived quicksort. Each comparison is a call through `compar`, so the comparison count decides most of the cost.

## Sources reviewed

- **C. A. R. Hoare, *Quicksort*** (Computer Journal 5, 1962): partition by exchanging from both ends.
- **R. Sedgewick, *Implementing Quicksort programs*** (CACM 21, 1978):
  - median-of-three pivots;
  - insertion sort for small subfiles;
  - recursing on (or stacking) the smaller part first, so the stack depth stays below log2 n;
  - stopping both scans on keys equal to the pivot, so equal keys split evenly instead of degrading to n².
- **J. L. Bentley and M. D. McIlroy, *Engineering a sort function*** (Software—Practice and Experience 23, 1993): the design behind the BSD `qsort` that picolibc and newlib inherit.
  - the ninther, a median of three medians, for large arrays;
  - swaps specialised by element size and alignment;
  - a warning that median-of-three alone can be driven to quadratic time by adversarial input (McIlroy's *A killer adversary for quicksort*, 1999).
- **D. R. Musser, *Introspective sorting and selection algorithms*** (Software—Practice and Experience 27, 1997): quicksort that switches a subarray to heapsort once the recursion depth passes 2·log2 n, which bounds the worst case at O(n log n). GCC's `std::sort` uses this.
- **O. Peters, *Pattern-defeating quicksort*** (2021, arXiv:2106.05123): introsort with adaptive refinements for sorted and patterned inputs. These were left out here for code size.
- **musl `qsort`:** smoothsort, read about in the P1.05 research. It is O(n) on sorted input, but it carries Leonardo-heap bookkeeping, and its random-input cost is close to heapsort's.

## Design (`libc/stdlib/speed/qsort.c`, the speed flavour only, D77)

- **Introsort:**
  - Ranges of more than 12 elements are partitioned around a pivot. The pivot is a median of three: the first, middle and last element, or a ninther above 64 elements.
  - The partition is Hoare's, both scans stopping on keys equal to the pivot, with the pivot parked at the range's start and swapped into place at the end.
  - Ranges of 12 or fewer are finished by insertion sort.
  - Each range carries a depth budget of 2·floor(log2 n). A range that exhausts it is heapsorted, the same sift-down as the size flavour's.
- **No recursion:**
  - The larger part of each partition goes on an explicit stack, and the smaller part is sorted first.
  - The smaller part is at most half the range, so the stack holds at most log2 n entries.
  - The stack has one entry per bit of `size_t`: 32 entries of 12 bytes on the 32-bit cores, 384 bytes of stack.
- **Swaps:**
  - one word when the element is a single aligned word (the `int` and pointer cases);
  - word by word when the element size is a multiple of the word and the base is aligned;
  - otherwise byte by byte.
- **Not stable**, as the standard allows (7.24.5.2), and as neither sort here is.

The size flavour keeps the heapsort: an introsort carries the heapsort as well, so it can only be larger.

## Tests

- **`test/unit/stdlib`**, every flavour:
  - **Correctness** against an insertion sort for every size up to 40 and several larger ones, for random, sorted, reversed, all-equal and two-valued inputs;
  - **Element sizes** of 1, 3 and 4 bytes, a word, and 40 bytes, at aligned and unaligned bases;
  - **The comparison count**, over each input of 1000 elements, is at most 3·n·log2 n (well above both sorts' typical counts, far below n²);
  - **An adversarial input:** McIlroy's killer, which gathers the pivots chosen by a median-of-three quicksort, built on the fly by a comparison function that settles each element's value only when first asked. It must stay within the same comparison bound, so the speed flavour is shown to take the heapsort path.
- **libc-test**'s `functional/qsort` runs in both flavours.

## Measured on the way

- **The first introsort** took random input 35–45 % below the heapsort's count, but had two regressions:
  - all-equal input was 162–179 % above the heapsort, which is nearly linear there because every sift stops at once;
  - reversed input was 6–12 % above on Arm.

  A check of the whole input for a run, before sorting, fixed both:
  - n − 1 comparisons for an input in order or all equal;
  - n − 1 comparisons and a reversal for an input in strictly descending order;
  - about two comparisons for any other input, since the run breaks at once.
- **The insertion-sort threshold:**

  | Threshold | `random/1024` | `random/16` |
  |---|---:|---:|
  | 6 | 361 | 159 |
  | 8 | 362 | 159 |
  | 12 | 370 | 187 |
  | 16 | 384 | 198 |

  Instructions per element on armv7e-m; 8 is the threshold kept.
- **Pointers instead of indices** in the scans, the insertion sort and the heap took `random/1024` from 362 to 274: each comparison no longer recomputes base + i·size.
- **The adversary and the run check:** McIlroy's adversary answers every comparison of the run check as "in order", so the check alone sorts it. The test therefore fixes its first two elements as a descending pair, which sends it into the partitioning. There it costs:

  | Sort | Comparisons, n = 1000 |
  |---|---:|
  | Size flavour (heapsort) | 16,046 |
  | Speed flavour (introsort) | 31,464 |
  | Speed flavour without its depth limit (mutant) | 87,069 |

  The test's bound is 40,000, so it fails the mutant.

## Results (speed flavour, instructions per element)

| Case | armv6-m | armv7e-m (and v7e-m-dp, v8-m.main) | x86-64 |
|---|---:|---:|---:|
| `qsort/random/1024` | 769 → 314 | 640 → 274 | 894 → 353 |
| `qsort/random/256` | 596 → 250 | 497 → 216 | 696 → 281 |
| `qsort/random/16` | 267 → 156 | 224 → 128 | 321 → 169 |
| `qsort/strings/256` | 708 → 356 | 585 → 303 | 617 → 274 |
| `qsort/sorted/1024` | 813 → 31 | 677 → 27 | 948 → 25 |
| `qsort/reversed/1024` | 723 → 42 | 602 → 36 | 837 → 50 |
| `qsort/equal/1024` | 139 → 32 | 114 → 27 | 157 → 25 |

Against picolibc and newlib-nano (`bench-runner compare`):
- on random input the counts are now level: 274 against 278 and 277 on armv7e-m, and 314 against 313 and 309 on Armv6-M;
- over all eight cases, picolibc and newlib-nano take 1.35–1.49 times the speed flavour's instructions (geometric mean), mostly through the ordered inputs.

Cost:
- `qsort.o` grows by about 625 bytes in the speed flavour's `libc.a`; no reference program links `qsort`;
- its stack depth grows from 68 to 508 bytes on armv7e-m, and from 84 to 524 on Armv6-M, mostly the 32-entry range stack.

The size flavour is unchanged.
