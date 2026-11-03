Kernel for more "privileged" instructions
Ring 0 = kernel, Ring 3 = user.

The kernel *is* memory sitting in physical RAM, like any other program.

Privilege is checked in two locations:
- Code segment register ("what code is running?"), which hardware checks on every privileged instruction.
- Page table entries, since the entire kernel physical entry in every address space

Function call (3ns) < Syscall (100-200ns) < context switch (3us)
IO syscalls take microseconds; blocking ones are unbounded.

# How syscall works: (is a fundamental instruction, NOT a function call)
- Changes to kernel privilege
- Saves current instruction address
- Sets next instruction to special address (in special register populated at startup)

Most of the actual work done by kernel entry code, such as:
- Save registers, switch to kernel stack
- Dispatches through the syscall table to the handler
Last part is a sysret instruction does the same in reverse.
Takes some 100ns

Kernel owns: CPU scheduling, virtual memory / page tables, networking, drivers, interrupts.

Interrupt vs syscall vs exception:
All transfer control to kernel
- Interrupt: by external device, so asynchronous
- Syscall: synchronous, deliberate
- Exception: synchronous, error (divide by zero) or control flow (page fault, stack growth)

Avoiding syscalls in hotpath: use kernel bypass, second option is busy polling + isolating cores
Can also use iouring (amortize syscalls by having queues for submitting/receiving)

vDSO (virtual dynamic shared object) -> optimize time commands to not need syscall
Trading systems usually elect to read cycle counter with **rdtsc()** directly.
(Can be reordered by CPU, pin thread)




