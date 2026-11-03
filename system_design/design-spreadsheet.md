Design a Spreadsheet
- You are building backend of a spreadsheet with cells like A1, B3
- setCell stores something in a cell, which can include references to other cells
- getCell evaluates the cell

Another framing:
Design a Computation Graph

Make sure not to go through the observations too fast!



## Initial clarifications:
Size supported?
Distribution of get/set calls?
What types are computed/stored?

## Logical Setup
We can represent dependencies as a graph. It will be a rooted DAG from the original cell downwards to its dependencies.

Can we have self-referential formulas; if not, probably best to return error at set time

## Naive Solution
We can find answer recursively
But inefficient for diamond-like patterns
Need to check for cycle dependencies. We can do a dfs from dependent cells to detect if original is reachable

## Optimized solution
Need to avoid redundant computations

Two main approaches:

Online correctness: topological propagation
- Writes recalculate all dependent cells by searching reachability and performing Kahn's over topological graph
- There is also clever argument that reversing postorder gives same guarantee, since a node only processed when all dependencies processed, but it is a bit too clever.

Offline correctness: memoized state (no wasted work)
- At each cell, have a dirty bit. If not dirty, our read just returns the stored value, otherwise we recurse and mark clean
- Writes mark everything upstream dirty, which requires an up graph as well

## Parallelization
- Two nodes are independent if one is not ancestor of another, so we can parallelize. Two approaches:
- Define level for computational nodes as furthest distance from some input node. Then we can process everything in same level simultaneously.
- Topological pushing: Only enqueue an element when its dependency count hits 0 in topological search. Needs a thread-safe queue for producer-consumer pattern. Better when costs are less uniform.

## Supporting Concurrency
- State in node should be locked behind mutex; we can use condition variable inside so that a requester can get notified
- Difficult case is if input changes mid-evaluation

- Simplest is lock the whole graph
- Next is to keep writes isolated, and have per-node {dirty, calculating, clean} state: when doing multiple computations, wait if the node is calculating by subscribing to condition variable.
- Complex things like copy-on-write snapshot, heuristics of just discarding/retry

Note this appears in real systems like TensorFlow, Bazel, Databases




## Impl details
How to support arbitrary formulas? (not just addition)
- Most flexible: store std::function, but that involves a virtual dispatch
- Improvement: parse expression once and store the root AST inside. Involves indirection but fine if we allocate nodes contiguously
- Best: Store items/operators in postfix order, resolve with a stack (think reverse polish notation)

