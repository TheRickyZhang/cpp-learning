Context:
Allocating physical memory (RAM) and virtual memory are separate events
mmap() will return a virtual memory address immediately, but not actually have any physical page mapped.
Page = chunk of virtual memory, frame = chunk of physical memory (typically same size)

Possible ways to handle page fault:
- Demand paging / Demand zero: think if this as flagging the page as "demand zero", where failing means we should get a zero-filled page block from RAM
- Copy-on-write (from fork())
- Read from disk: happens when file-backed mapping, slowest. Will sleep.
- SIGSEGV: error

Note, after a page fault, the instruction is re-executed, which updates the TLB


General shape: TLB vs Page Table vs retrieve from memory
Dereference path: virtual address -> TLB -> page table walk -> physical page -> cache hierarchy
- TLB miss != page fault. TLB miss is cheap, since just hardware involved in walking page table.
- Page fault will trap to kernel: possibilities are allocating a frame, reading
- Not in page table = page FAULT.

Page FAULT {
  - Find data in page cache (subset of RAM)
  - Otherwise, fall back to disk
}

To improve efficiency (noting that internal structure is trie):
- Huge pages (decrease a level, add more nodes at bottom)
- Cache prefix key -> direct lower levels instead of walking the tree (page-walk caches)

When implementing:
- Linux can handle huge pages automatically with transparent huge pages, can also enable explicitly (preferred for lower latency)
- Huge pages = more internal fragmentation (might only use a small amount of a huge page). Also more difficult to get contiguous memory from kernel.

Caches are indexed by physical addresses, as otherwise it would be ambiguous where the same virtual address can map to different physical addresses in different processes.
- An exception is that the L1 cache is virtually indexed, physically tagged because we can guarantee that virtual -> physical mapping keeps the last 12 bits untouched, so they would always go to the same set across different processes


Attempt:
Alright. So I think I've got a basic idea of how, uh, the virtual just versus physical just page table TLB works. I'll give an overview again here just to make it interview ready. So, uh, each process has a virtual address space, um, which corresponds to some physical address, which is basically to keep the processes own memories sort of distinct from each other So I'll have a wait to ground that in the actual memory. If when you perform something like dereferencing a pointer, we're going to be trying to, like, get the actual memory. One possibility is it could find the entry in the TLB, which is a cache for virtual two physical page table mappings, and it's done. If that fails, then I'll look at the page table or the processes page table. and it can find the, uh, mapping in there. If that fails, then it will trigger a page fault, um, which could be one of multiple things such as, um, needing to zero... getting a zeroed out page where the, um, uh, you can get virtual memory without actually setting up the physical memory there. And so it'll sort of do it lazily upon the page fault. You can also have... because it's, like, copy on right from when you've forked a process, and it's a similar idea of we want to get the corresponding value lazily. And there are some cases where it's more of an error, and the... you'll get, like, a c serve the terminal or a command. So once... if you do get a page fault, then it will write the corresponding entry in the page table. Um, and then the instruction will be run again so that the correct page is fetched and the PLB is updated.

Solid — that's close to interview-ready. Four fixes.

You skipped the file-backed case in your fault list. Page from disk or page cache, from an mmap'd file or the executable itself. Worth including.

Second, the page table isn't searched — it's walked. Four levels, indexed by address bit chunks. Say "walk," not "look in."

Third, small wording thing: SIGSEGV is a signal, not a terminal command. Default action is terminate.

Fourth, "get the corresponding value lazily" for COW is slightly off. The frame already exists and is mapped — it's a write to a read-only shared page that triggers the copy. It's a permission fault, not a missing mapping. Worth stating separately since it's the one case where the PTE was present.

o
