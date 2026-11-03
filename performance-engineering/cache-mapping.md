# How is a cache set up, and how an address maps to an entry in the cache
This is default shape for all hardware lookup structures (cache, TLB) because it takes short fixed time, no chaining or probing.

Virtual memory is usually 48 bits
- Easy to standardize because it's flexible; for instance, you can mmap a file to your virtual address space
Physical memory is usually 40 - 50 bits (1TB - 1PB), limited by how much is available in hardware

Each set contains some number of ways
A cache maps sets (index bits) to some number of entries (ways)
Each entry stores:
- tag, for identification
- cache line data
- valid/dirty bit

N-way associativity = how many ways/set.
- Direct-mapped = 1 way / set, Fully-associative = 1 set (ways == # lines)

Addresses are tag / index / offset
- Offset makes sense - corresponds to size of a cache line
- Index right above offset means that consecutive cache lines map to different sets, which works well for most programs
- Tag corresponds with set associativity: 

## Walkthrough with typical modern software:
Typical L1 data cache is 32KB and 8-way

Line size is 64 bytes -> 6 offset bits
So 32K / (64 * 8) = 64 sets -> 6 index bits

Since physically tagged, we have 48 - (6 + 6) = 32 tag bits.

## Intuition for virtually indexed, physically tagged
In the above example, we have 12 index + offset bits.
Typical pages are 4 KB = 12 bits. So virtual -> physical page mappings will preserve the lower 12 bits, also preserving the index in L1 as well.




