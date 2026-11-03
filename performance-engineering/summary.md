
From course -> detailed

Benchmarking: how fast is it under specific workloads?
Profiling: Where are time and resources going?
Tracing: What was the exact order?

Instrumentation / observability: measurement and analysis of data collection

# Optimizations worth learning, ranked
| Optimization | What you'd actually change | Priority |
|---|---|---|
| Cache locality | Replace pointer-heavy structures with contiguous arrays; improve traversal order | Very high |
| Memory layout | AoS -> SoA, reduce struct size, reorder fields, split hot/cold data | Very high |
| Branch prediction | Remove unpredictable branches, optimize common paths | Very high |
| SIMD / vectorization | Make loops vectorizable; use intrinsics when necessary | High |
| Cache blocking / tiling | Process matrices or arrays in cache-sized chunks | High |
| Allocation reduction | reserve(), reuse buffers, avoid per-element allocations | High |
| Instruction-level parallelism | Unroll loops, use independent accumulators | Medium |
| False sharing | Separate frequently written per-thread data into different cache lines | Medium |
| Instruction-level tuning | Replace expensive operations, inspect assembly, reduce dependencies | Medium |
| Prefetching | Fetch data before it is needed | Low-medium |

## AoS -> SoA
AoS is bad primarily because of memory inefficiency: every byte touched must be transferred CPU <-> DRAM at some point.
Fetching all fields of a struct when only one is needed wastes bandwidth and can make the program memory-bound.
