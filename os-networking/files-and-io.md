File descriptor = abstraction over IO, index into table
Table = in KERNEL

-> file description = high-level information: cursor offset, access mode (read/write/both), status flags, read()/write()/close() overrides
- When creating a file / socket / device, the corresponding handling functions are known at compile-time, so we store a pointer to a struct of function pointers in the fd table
- Not all fields are used by all types - for instance, offset is meaningless for a socket

# Reading a file
Initially, file data is in disk, not physical memory (RAM).

- Address translation: hardware raises exception (page fault), restarts instruction
- File reading: user syscall (not found in page cache), fetches from disk

Two unrelated things named "page", both dealing in frames:
- Page table: virtual address -> physical frame. One per process.
- Page cache: inode + offset -> physical frame (via page descriptor). One for the whole system.

If not in page cache, thread makes request to disk, blocks itself (thread adds itself to wait queue, marks as blocked). Then the disk's interrupt handler does waking, and the thread memcpys to buffer. (Generally, reads from disk need to be blocked)

Disk -> page cache is DMA (no CPU copy). Page cache -> user buffer is a CPU memcpy; mmap avoids it by mapping the page cache frame directly.

# read() vs mmap()
Same pages end up in the page cache either way. The difference is how many kernel transitions, and what each costs.

| | read() | mmap() |
|---|---|---|
| Kernel entries | 1 syscall per chunk (you pick size) | 1 page fault per first-touched page (4KB) |
| Copy | page cache -> your buffer | none |
| Your buffer | mapped once, reused, no faults | every page faults: trap + page cache lookup + PTE update |
| Readahead | sequential pattern detected, next pages prefetched | also present (fault-around), but weaker |

- Cost per fault ~1us; big file = 100k+ faults, usually more than the memcpy saved (warm memcpy is very fast per byte).
- read: pays copy, amortises kernel transition over a large chunk -> sequential streaming.
- mmap: skips copy, pays transition per page -> random access, repeated access, or sharing between processes (ex databases).
- Mitigations for mmap: MAP_POPULATE (prefault everything up front), madvise(MADV_SEQUENTIAL / MADV_WILLNEED), huge pages (fewer faults).
- Fault cost breakdown: see virtual-to-physical-page.md. Syscall cost numbers: see kernel-vs-user-mode.md.

O_DIRECT bypasses the page caches - good for not evicting page cache
Logging typically goes to preallocated mmapped region, and separate thread flushes it.

# Reading a socket
Normally blocks, O_NONBLOCK returns -1 immediately

`poll`: Pass entire fd list on every call. (Max is 1024 fds, bitmask)
- 
`epoll`: register once with `epoll_ctl`, `epoll_wait` will return ready count and the ready fds.
- epoll syntax itself is not about reading, that is always done by read(). It instead will give you nonempty sockets that won't block when reading, so that you are guaranteed to not waste work.

- Generally prefer state-change triggered vs state-triggered, since fewer wakeups



Other:
fsync() actually writes to disk instead of lazily copying from physical memory
