1. Processes & threads
process vs thread, address spaces, fork, exec, clone, thread creation, process states

Different across everything:
- Program counter, stack pointer, registers
- Scheduling priority (own nice value, scheduling class)
- thread_local storage (more granularity, avoids contention)

Shared across threads, different across processes:
- Address space
- File descriptor / signal handler table

Shared across processes:
- Physical memory
- Can use shared memory and sockets to communicate

# Context switch (always between threads)
Saved/restored: registers, stack + instruction pointer, pointer to task_struct / page table (pointer swap, not copy).
Indirect costs, largest first:
- Cache warmth: tagged by physical address so no flush, but the new working set evicts the old (ping pong penalty)
- TLB: only valid in one address space, so a cross-process switch flushes it (PCID keeps entries but still pays eviction)
- Branch predictor state

# fork() / exec()
- fork() creates a process (NOT a thread): new address space, copy-on-write, so a page is copied only on the child's first write fault
- Only the calling thread exists in the child
- Usually followed by exec() to replace the image: a shell runs `ls` by fork() then exec("/usr/bin/ls")
- Libraries use fork(); avoid it directly in user code
