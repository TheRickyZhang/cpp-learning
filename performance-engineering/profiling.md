# Top-down analysis
Profilers bucket every micro-op slot into four categories (see algorithmica-notes.md for typical percentages):
- Retiring: useful work
- Bad speculation: work thrown away
- Frontend-bound: backend ready, no micro-op to give it
- Backend-bound: core-bound (execution units) or memory-bound

## Frontend
Instructions are bytes in the executable's memory, in DRAM.

Instruction fetch pulls bytes from these addresses, which takes time to reach the CPU
- Hot instructions get cached (address -> bytes) in the instruction cache to skip the trip

Instruction decode converts bytes to micro-ops, basic tasks that take a single cycle
- Separates ISA compatibility from the CPU-specific implementation
- Easier for the backend to handle abstractly

Branch prediction uses local 2-bit counters plus a fixed-size branch history table.

## Backend
Four stages:
- Register renaming: maps to the many physical registers so unrelated instructions run in parallel
- Scheduler: holds micro-ops until inputs are ready
- Execution units: do the work
- Reorder buffer: micro-ops must retire in original program order

Tomasulo's algorithm:
- Register -> reservation station mapping
- Ops waiting on an input get notified over the common data bus when it becomes ready

## Frontend-bound
Perf symptoms: many instruction cache / iTLB misses

Causes:
- Branchy code
- Too much inlining (code bloat)
- Hard-to-predict jumps (e.g. virtual calls)

Branchless programming (relies heavily on cmov), effective when branches are unpredictable:
- `if (cond) count++`        -> `count += cond`
- `if (cond) a.push_back(x)` -> `a[i] = x; i += cond`   (stream compaction)

## Backend-bound
- Dependency chains -> multiple accumulators / independent work = exploiting instruction-level parallelism
- One execution unit overloaded -> avoid that operation or convert to an alternative
- A long load can fill the reorder buffer: every later instruction waits for it to retire, stalling everything
