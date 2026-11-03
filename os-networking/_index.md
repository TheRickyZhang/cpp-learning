# OS / networking index
| # | Topic | Covers | File |
|---|---|---|---|
| 1 | Processes & threads | process vs thread, address spaces, fork, exec, context switch | processes-and-threads.md |
| 2 | Virtual memory | translation, page tables, TLB, page faults, demand paging, COW, huge pages | virtual-to-physical-page.md |
| 3 | Syscalls / kernel boundary | user vs kernel mode, syscall vs interrupt vs exception, vDSO | kernel-vs-user-mode.md |
| 4 | Scheduling | thread states, blocking cost, affinity, isolcpus, nice / SCHED_FIFO | threads-and-scheduling.md |
| 5 | Concurrency (OS view) | futex path, contention cost, wakeups, deadlock | futex-and-wakeups.md (C++ API: ../concurrency/) |
| 6 | Files & I/O | fds, page table vs page cache, read vs mmap, O_DIRECT, fsync | files-and-io.md |
| 7 | I/O waiting | blocking/nonblocking, poll/epoll, level vs edge | files-and-io.md (socket section) |
| 8 | Interrupts & devices | IRQs, softirqs, DMA, PCIe, interrupt affinity | GAP (DMA only: ../performance-engineering/kernel-bypass.md) |
| 9 | IPC & signals | pipes, shared memory, signals, unix sockets | signals-and-ipc.md |
| 10 | Filesystems / storage | inode, dentry, path lookup, journaling | filesystem.md |
| 11 | Performance | cache locality, false sharing, NUMA, context-switch cost | ../performance-engineering/ |
| + | Timekeeping | vDSO, rdtsc pitfalls | timestamping.md |
| + | Linking / loading | compilation pipeline, ELF, what happens on ./a.out | compilation-and-linking.md |
| + | Userspace allocation | glibc arenas, brk vs mmap, why preallocate | ../memory/malloc-free.md |
| + | Real-time scheduling | SCHED_FIFO, isolcpus, nohz_full; priority inversion is a GAP | threads-and-scheduling.md |
| N | Networking | sockets, TCP vs UDP, multicast, kernel bypass | networking-summary.md, sockets.md, tcp-udp.md, ../performance-engineering/kernel-bypass.md |

Rule: OS mechanism lives here. Measurement and optimization technique lives in performance-engineering/. C++ API lives in concurrency/ and memory/.

# Mock interview prompt and gap analysis
Act as a mock interviewer for a low-latency C++ quantitative developer role. Interview me the way a real interviewer would: one question at a time, follow-ups based on my answers, no hints unless I'm clearly stuck.

Emphasis: operating systems and networking fundamentals, with C++ and systems design as needed. Start with fundamentals and go deeper only once I've demonstrated them. I will provide the topics.

Rules:

If I say anything incorrect or imprecise, stop and correct it before moving on.
Probe the edges of my knowledge — push until you find where my understanding breaks down.
Don't accept vague answers; ask for mechanisms, tradeoffs, and numbers where relevant.
Stay in character; no summaries or encouragement mid-interview.

Goal: find and expose my gaps so I'm ready for real interviews.

1. Operating systems

You should be able to explain
1. Processes & threads	process vs thread, address spaces, fork, exec, clone, thread creation, process states
2. Virtual memory	virtual → physical translation, pages, page tables, TLB, page faults, demand paging, COW, mmap, swap
3. Syscalls / kernel boundary	user vs kernel mode, syscall vs interrupt vs exception, entering/exiting kernel
4. Scheduling	runnable vs blocked, context switch, preemption, priorities, CPU affinity, scheduler latency
5. Concurrency	mutexes, futexes, atomics, memory ordering, spinlocks, condvars, semaphores, deadlock
6. Files & I/O	file descriptors, open file descriptions, read/write, page cache, mmap, buffered I/O
7. I/O waiting	blocking/nonblocking I/O, select/poll/epoll, level vs edge triggering
8. Interrupts & devices	IRQs, softirqs, DMA, PCIe, interrupt affinity
9. IPC & signals	pipes, shared memory, signals, sockets, wakeups
10. Filesystems/storage	inode/dentry basics, page cache, fsync, persistence; journaling at a high level
11. Performance	cache locality, false sharing, NUMA, context-switch cost, page faults, profiling/tracing

Short answer: your OS list is already broader than what most knowledge-check rounds sample from. The bigger risk is what's not on it — C++ itself, computer architecture, and the "glue" topics that sit between OS and the trading system. Postings for these roles consistently pair Linux internals with cache behaviour, SIMD, profiling tools and TCP/UDP, multicast, kernel bypass or exchange protocols, and some explicitly name lock-free data structures and NUMA, CPU affinity, and custom memory management. 
C++ Quant Developer +3

Gaps within OS (small, add these)

Userspace allocation: malloc internals (glibc arenas, brk vs mmap threshold, fragmentation), why HFT uses pools/arenas, huge pages (THP vs explicit hugetlbfs) and TLB reach.
Timekeeping: TSC vs HPET, clock_gettime via vDSO, rdtsc/rdtscp pitfalls, why "time a syscall" is a trick question.
Linking/loading: static vs dynamic, ELF, PLT/GOT overhead, "what happens when you run ./a.out."
Real-time scheduling: SCHED_FIFO, priority inversion, isolcpus/nohz_full, why you still get jitter on an isolated core.

Missing categories (large — spend most time here)

Computer architecture with numbers: cache line size, L1/L2/L3/DRAM latencies (~1/4/10–40/80–100 ns), cross-socket penalty, branch misprediction cost, store buffers (this is what makes seq_cst vs acquire/release concrete), prefetchers, hyperthreading tradeoffs, memory bandwidth. Interviewers ask for numbers here more than anywhere else.
C++ core: move semantics, copy elision/RVO, virtual dispatch cost vs CRTP/templates, constexpr, alignment/padding, exceptions cost, placement new, allocators, likely/__builtin_expect, ODR/inlining, UB the compiler exploits. Plus implementation questions: SPSC ring buffer, spinlock with backoff, shared_ptr internals, small-vector/flat-map.
Networking (you're bringing this, but the usual misses are): NIC ring buffers and RSS, kernel bypass mechanics (Onload/ef_vi/DPDK — what gets skipped and why that's faster), hardware timestamping/PTP, multicast joins and gap recovery, Nagle/TCP_NODELAY, socket buffer sizing, busy polling.
Trading-domain system design: order book data structure choices, feed handler → strategy → order gateway pipeline, where each latency source lives. The integration question "walk me from packet-on-wire to order-out" ties OS, net, arch, and C++ together and exposes gaps a topic-by-topic drill won't.

Depth vs breadth

Go deeper on OS: pros — finds where you break under 3+ follow-ups; cons — diminishing returns, interviewers rarely push past a page-table walk or a futex wait/wake path in a 45-minute slot.
Broaden into the categories above: pros — knowledge checks sample across topics, and a blank on "what's cache line size" costs more than a shaky answer on journaling; cons — shallower per topic.
Hybrid (my recommendation): breadth on 1–4 above, and get OS depth for free by building rather than re-reading — write an SPSC queue, pin it to two cores, measure with rdtsc, look at it in perf, then explain why the numbers came out that way. That forces false sharing, memory ordering, scheduler jitter, and TLB effects to become things you've observed, which is how a real interviewer tells memorized from understood.

Within OS, the four areas worth a second deep pass because they overlap the job most: virtual memory (huge pages, page-fault cost), concurrency/atomics (futex path, memory model), scheduling/isolation, and the I/O path from NIC to userspace. Filesystems and IPC can stay at the level you have.
