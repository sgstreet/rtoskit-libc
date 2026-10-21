# Research: uncontended lock fast path

Step: P3.04a. Decision: PLAN D79 (amends D31).

## Where the time went (P3.03b speed baseline)

Every `malloc` and `free` takes `__rtos_malloc_lock`, and every stdio call
takes its stream's lock, through `__rtos_lock_acquire` and
`__rtos_lock_release` (D31). The Linux port's acquire:
- calls `__rtos_thread_self` (a TLS load);
- compares the owner for recursion;
- does a compare-and-swap;
- stores the owner and the depth.

Its release repeats the owner check and does an exchange. Replacing both
with empty functions measured the cost (instructions per operation):

| Case | armv6-m | armv7e-m |
|---|---:|---:|
| `malloc` + `free` (16 bytes), with locks | 233 | 107.5 |
| the same, with no-op locks | 54 | 46.5 |

Locking is 57 % of an allocation on armv7e-m and 77 % on Armv6-M, where
each atomic operation goes through libc's atomics helpers to the port's
compare-and-swap.

## Sources reviewed

- **Linux priority-inheritance futexes**, `futex(2)`, `FUTEX_LOCK_PI`: the
  futex word holds the owner's thread ID, plus a `FUTEX_WAITERS` bit.
  User space takes a free lock with a compare-and-swap of 0 to its TID and
  releases one with a compare-and-swap of its TID to 0. The kernel is
  called only when that fails, and it can boost the owner because the word
  names it.
- **U. Drepper, *Futexes Are Tricky*** (2011): the three-state mutex the
  ports use today, and why the waker must be told that someone may sleep.
- **musl**, `pthread_mutex_trylock.c` and `pthread_mutex_lock.c`
  (<https://git.musl-libc.org/cgit/musl/tree/src/thread/pthread_mutex_trylock.c>):
  - the lock word holds the owner's TID and a waiters bit;
  - a recursive lock held by the caller only bumps a count;
  - a normal mutex is a single compare-and-swap.
- **glibc** `lowlevellock.h` and the recursive `pthread_mutex_lock`: the
  same idea, an inline compare-and-swap and a call into the kernel only on
  contention, with the owner TID stored beside the word.
- **newlib and picolibc retargetable locks**
  (`__retarget_lock_acquire_recursive`): every operation is a call to the
  OS port, with no fast path. This is the design D31 followed.
- **Zephyr `k_mutex`**
  (<https://docs.zephyrproject.org/latest/kernel/services/synchronization/mutexes.html>)
  and **FreeRTOS** recursive mutexes: kernel calls with priority
  inheritance. An RTOS port needs the owner's identity to do priority
  inheritance.

## Design

The lock word becomes part of the seam contract (`rtos-port.h`):

- **The word:** `struct __rtos_lock` starts with an `unsigned int`. It holds
  0 when the lock is free. Otherwise it holds the owner's token, with
  `__RTOS_LOCK_SLOW` (bit 0) set when releasing needs the port: the owner
  holds the lock more than once, or another thread may be waiting.
- **The token:** the thread's value of `__rtos_lock_token`, a
  `thread_local unsigned int` that the port defines. It is nonzero, even,
  and unique among the threads that may hold a lock.
  - Reading it is one TLS load, which is cheaper than
    `__rtos_thread_self()`.
  - The port chooses the value, so an RTOS port can make it name the
    thread, as the TID does for a PI futex, and implement priority
    inheritance in its slow path.
  - The Linux port gives the initial thread 2, and each new thread the next
    even number when it starts.
- **Where the fast path runs:** in the port's functions, as their first
  compare-and-swap. libc still makes one call per operation in both
  flavours. The contract lets libc do the compare-and-swap in line instead,
  and `test/unit/locks` exercises that path, but libc does not use it:
  - Measured in the speed flavour, the in-line check took malloc + free on
    armv7e-m from 92.5 instructions (the port's fast path) to 67.5, on
    x86-64 from 84 to 54.5, and on Armv6-M from 224 to 192.
  - It also added 25–35 bytes per call site. That broke the speed
    flavour's +10 % budget by up to 200 bytes in every malloc and stdio
    image (`malloc-heavy-tiny` had about 5 bytes of headroom, Q16).
  - It stays available to a later flavour, or to a larger budget, without
    another seam change.
- **The port's slow path (Linux):**
  - **Recursion:** the owner counts nested acquisitions in `depth` and sets
    the slow bit, so its releases reach the port. The last release clears
    the word and wakes one waiter if the bit was set; that wake can be
    spurious after recursion, which is harmless.
  - **Contention:** another thread sets the slow bit and sleeps on the
    futex until the word is 0. It then takes the lock with the slow bit
    already set, since more threads may be waiting. This is Drepper's
    third state.
  - **Misuse:** releasing a lock held by another thread, or not held at
    all, aborts, as before.
- **Single-threaded ports** (`port/none`) keep no-op functions. No other
  thread can observe that a nested release clears the word early.

### Trade-offs

- **Armv6-M:** the compare-and-swap is a helper call into the port's
  critical section (here the Linux kernel's `cmpxchg` helper), so the gain
  is smaller.
- **Code size:** the size flavour allows no growth. The Linux port's
  functions fit by using one compare-and-swap in the acquire loop, a plain
  store for the owner's slow bit, and one exchange in the final release;
  `__rtos_thread_self` and the owner pointer are gone. Images shrink by 0
  to 62 bytes of code and 36 to 96 bytes of RAM. On Armv6-M the acquire
  frame grows from 32 to 40 bytes, which no reordering avoided without
  growing the code: `__atomic_compare_exchange_4` takes five arguments, and
  four values are live across it.
- **Tokens:** they are 32 bits so that the word can be a futex on 64-bit
  Linux too. The Linux port's counter wraps after 2^31 thread creations.
- **`mtx_t`** is unchanged. It already does its compare-and-swap in line,
  and reaching it through the token would change `<threads.h>`.

## Tests

- **`test/unit/locks` (new):**
  - the word's states after each step;
  - nesting through libc's wrappers and the port's functions in any mix;
  - a token per thread, distinct and even;
  - four threads adding under one lock, nested every third round, through
    either path, yielding while they hold it.
- **`test/unit/threads`** still runs malloc and stdio under contention.
- **libc-test** runs on every board and flavour.
