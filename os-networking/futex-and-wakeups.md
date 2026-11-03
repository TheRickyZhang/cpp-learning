# Mutex / futex
An atomic RMW (fetch_add, CAS) is enough for a counter. A mutex makes a region exclusive; waits can be arbitrarily long, so losers must sleep.
- Futex = boolean flag in user memory, CAS to lock
- Uncontended pass: ~10ns, no kernel
- Fail: syscall with the flag address + this thread; kernel rechecks the flag, then queues the thread on that address
- Unlock with waiters: syscall to wake one
- Kernel is only involved to sleep / wake, never on the uncontended path
- Spinlock = simplest lock, never sleeps; only for tiny critical sections on dedicated cores

# Cache contention
Uncontended atomic ~20ns -> contended ~200ns (line bounces between cores).
False sharing = same cost with no logical sharing; fix with alignas(hardware_destructive_interference_size).

# Wakeups (condition variables)
cv.wait must be in a while loop:
- Spurious wakeups: woken with the condition still false
- Stolen wakeups: a third thread takes the mutex between notify and reacquire

# Deadlock
Circular wait needs mutual exclusion, hold-and-wait, no preemption. Fix with global lock ordering.

Memory ordering: see concurrency/atomics_and_memory_order.md.
