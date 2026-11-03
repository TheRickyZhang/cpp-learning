What happens when you call malloc?

Multi-structure checks:
Thread-cache = lock-free linked list
Regular bins = free lists: involves searching by size, combining adjacent chunks, locked per arena.

If an arena does not have a suitable free chunk, glibc goes to the kernel and calls one of these two based on some threshold (both just kernel calls to mark virtual addresses valid)

*brk* = Just grow heap breakpoint, so you "take" some memory from the kernel
- used for small allocations
- Just moves in virtual address space, so when code actually uses pages, raises page fault
- Gap between heap and stack exists on purpose since we don't know exact split, and it also helps catch bugs
- Syscall amortized by handing over small chunks

mmap = ask kernel for virtual address anywhere in the unused region
- used for larger allocations
- expensive syscall

Preallocate, since memory allocation has a variable cost, since it potentially involves searching through free lists, syscalls, page faults on first touch, and lock contention

Returned pointer is always 16-byte aligned.
