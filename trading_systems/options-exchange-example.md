# Options Exchange Design Notes

A price-time options exchange in C++ on Linux. The main goal is the exchange protocol and wire spec; the architecture around it is kept simple for a first pass, with latency, throughput, and fairness in mind.

The notes follow a message along its path: requirements → mental model → protocol → ingress → matcher → outputs → replay → hardware → environment → build plan.

---

## 1. Requirements

**Inputs loaded at startup:** the options universe to trade, and a client whitelist.

| Area | Requirement |
| --- | --- |
| Connections | TCP, only whitelisted clients, one session each |
| Order types | DAY and IOC, limit only. New and cancel (no cancel-replace) |
| Quotes | Two-sided, atomic both-side replace, post-only, exactly one resting quote per (client, symbol) |
| Matching | Price-time, per option |
| Market data | Multicast feed: trades and top 3 levels per side per symbol |

**Required messages** (client → exchange / exchange → client):

- Logon / logon response
- New Order / ACK
- Cancel Order / order status (fill, ack, expired)
- Mass quote (two-sided) / ACK

**Additional messages worth having:** heartbeat, logout, mass cancel, resend / snapshot requests, trade correction.

The exact wire format (fields, sizes, framing, sequence numbers, error codes) is ours to design.

**Protocol design points:**

- What messages do we need?
- What types do we use?
- How to handle protocol errors?
- How do we know clients are up?
- What's the wire format and framing?

**Cross-cutting concerns to keep in mind:**

- Recording: logging, auditing, replay and determinism
- Observability
- Threading and handoff
- Hot-path discipline
- Startup and shutdown
- Failure handling
- Testing and tooling

**Process:** trace the full path of every supported message and separate the steps into logically independent categories. Once that picture is clear, think about throughput and fairness, which are the main architectural challenges (dedicated threads, load balancing, etc.).

---

## 2. Mental Model

A matching engine project is not just an order book. It is a full pipeline:

```text
input data
  -> deterministic event sequencing
  -> order-book state transition
  -> execution generation
  -> market-data publication
  -> replay/debug/recovery tooling
```

The most important invariant:

```text
same ordered input stream -> same exact output stream
```

This is why many engines use a **single-writer event loop** per symbol, product, or partition.

The engineering themes that follow from it:

- determinism;
- sequencing;
- latency;
- reproducibility;
- observability;
- controlled networking;
- controlled hardware placement.

---

## 3. Protocol

Separate a session layer (TCP) from an application layer (orders/quotes).

### Session Layer

- Login / ACK
- Heartbeat
- Logout
- SessionReject + disconnect (if violating protocol)

### Application Layer

- New Order → ACK
- Cancel Order → ACK
- Mass Quote → ACK per symbol
- Mass Cancel → ACK
- Reports: Filled, Expired (IOC / EOD), Cancelled (from mass cancel or disconnect)

### Wire Structs

```cpp
// Often times might pad for alignment - implementation detail, add later
struct Login {
  char     client_id[8];
  char     token[16]; // Something like client_id + password
  uint32_t last_seen_seq;
  uint32_t _pad;
};
struct NewOrder {
  uint64_t cl_ord_id;
  uint32_t option_id;
  uint8_t  side;
  uint8_t  is_IOC;
  uint16_t _pad;
  int64_t  price;
  uint32_t qty;
  uint32_t _pad2;
};
struct OrderAck {
  uint64_t cl_ord_id;
  uint64_t order_id;
  uint64_t ts;
};
struct Fill {
  uint64_t cl_ord_id;
  uint64_t order_id;
  uint64_t exec_id;
  uint64_t trade_id;
  int64_t  price;
  uint32_t fill_qty;
  uint32_t leaves_qty;
  uint8_t  is_maker;
  uint64_t ts;
};
struct MassQuote {
  uint64_t quote_msg_id;
  uint16_t count;
  struct Entry {
    uint32_t instrument_id;
    uint32_t _pad;
    int64_t  bid_px; uint32_t bid_qty;
    uint32_t ask_qty; int64_t ask_px;
  } entries[count];
};
```

---

## 4. Architecture: Threads and Rings

```text
Client sessions (identity implicit in which socket the bytes arrived on, i.e. ip/port)
  -> Kernel TCP stack (sockets + epoll)
  -> Ingress thread
  -> Ingress SPSC ring
  -> Matcher/Sequencer thread (sharded, e.g. by underlying symbol first letter)
       -> journal ring -> journal thread
       -> TCP ring     -> order-entry publisher thread
       -> MD ring      -> market-data publisher thread
```

### Ingress Thread

- Set up sockets.
- Service each socket round-robin with a fixed budget: fair, nonblocking.
- Buffer to reassemble messages.
- Handle login state.
- Decode + timestamp.

### Matcher/Sequencer Thread

Per message, in order:

1. Push to journal ring.
2. Validate.
3. Update book.
4. Push to TCP ring (responses to clients).
5. Push to MD ring.

### Failure Policy per Ring

| Ring | If the push fails |
| --- | --- |
| Journal | Must not fail; shut down the exchange |
| TCP (order entry) | Disconnect the client |
| Market data | Drop |

### Startup Order

load universe → assign instrument IDs → preallocate books/pools → start journal → start MD (publish an instrument directory, see §6) → open listen socket → accept logins → set state OPEN

### Latency and Fairness Notes

- Most latency comes from kernel wakeup plus the two ring hops.
- Prefer journaling on a separate thread and ACKing immediately.
- Later fairness: reorder based on ingress timestamps instead of relying on queue order.

---

## 5. Matching Engine and Order Book

The matcher is a deterministic order-book state machine with strict sequencing and reproducibility.

**Operations.** This spec needs: add limit order, cancel, two-sided quote replace, partial and full fills, trade reports, book updates, order ID lookup, and per-symbol book partitioning. General-purpose engines also handle replace/modify and market orders, which are out of scope here.

**Book structure:**

```text
Order ID map:
  order_id -> order pointer / metadata

Bid book:
  price -> FIFO queue of buy orders

Ask book:
  price -> FIFO queue of sell orders
```

**Price-time priority:**

- best bid = highest buy price;
- best ask = lowest sell price;
- earlier order at same price has priority;
- aggressive orders match resting orders on the opposite side, trading at the resting order's price.

**Implementation choices:**

- `std::map` / tree by price: easier, slower but fine for learning;
- flat arrays by tick index: faster if price range is bounded;
- intrusive linked lists: common in high-performance designs;
- object pools: avoid hot-path allocation.

---

## 6. Market Data over UDP Multicast

UDP multicast is common because one publisher can send to many subscribers with low overhead. Market data tends to be published before the order-entry confirmation, or at the same time.

**Typical message fields:**

```text
sequence number
message type
symbol id
side
price
quantity
order id or level id
timestamp
```

**Feed pattern:** snapshot + incremental updates. The snapshot gives the full book state; incrementals update it afterward.

**Receiver responsibilities:**

- track expected sequence number;
- detect packet gaps;
- request recovery or reload snapshot;
- reconstruct book state;
- handle duplicate or out-of-order packets if feed design allows them.

---

## 7. Replay and Determinism

The useful abstraction:

```text
input event log
  -> decoder
  -> deterministic engine
  -> output event log / market-data feed
```

A replay system reads a historical or canned event file and reconstructs a stream of inputs. It is used for testing, simulation, debugging, and performance validation.

Important properties:

- same input should produce same output;
- events should be sequenced;
- timestamps may be replay-time, simulated-time, or original exchange-time;
- output should be checkable against expected fills/book updates;
- failures should be reproducible from logs.

---

## 8. Hardware and Networking

### CPU / NUMA / Physical Hosts

Physical engine boxes matter because latency-sensitive systems care about jitter. They give better control over CPU isolation, NUMA locality, and NIC placement.

- **CPU pinning**: keep hot threads on specific cores;
- **NUMA locality**: keep thread, memory, and NIC close to each other;
- **IRQ affinity**: control which cores handle NIC interrupts;
- **isolated cores**: reserve cores for hot-path processes;
- **cache locality**: avoid bouncing data across cores;
- **false sharing**: avoid unrelated hot variables sharing a cache line.

More cores do not automatically make a matching engine faster. Often the bottleneck is ordered state mutation, so a single book/partition may be intentionally single-threaded for determinism.

### Kernel Bypass

Do not over-focus on full RDMA semantics at first. The likely relevant concept is **kernel bypass packet I/O**.

```text
Normal socket path:  application -> syscall -> kernel networking stack -> NIC
Kernel bypass path:  application -> NIC queue directly / mostly directly
```

Common approaches: RDMA, raw Ethernet queue pairs, DPDK, AF_XDP, busy polling. Mellanox / NVIDIA NICs are common here and support RDMA, RoCE, raw packet queues, hardware timestamping, and offloads.

Vocabulary:

- **NIC**: network interface card;
- **QP**: queue pair;
- **CQ**: completion queue;
- **MR**: memory region;
- **RoCE**: RDMA over Converged Ethernet;
- **raw Ethernet QP**: send/receive raw packets through verbs;
- **busy polling**: spin instead of blocking to reduce latency;
- **huge pages**: large memory pages, often used in high-performance packet I/O.

Important distinction: Mellanox + QP does not always mean one-sided RDMA reads/writes. It may just mean raw packet TX/RX with lower overhead than normal sockets.

---

## 9. Environment Context: TSIM / Andromeda

The surrounding project looks like a **simulation / replay environment around a low-latency matching engine**:

```text
.ncr file
  -> market-data replay
  -> market echo agent
  -> orders / events
  -> TSIM Andromeda matching engine
  -> multicast UDP output
```

### Likely Host Layout

```text
outside network
  -> service/gateway box
  -> controller / orchestration
  -> physical engine box
  -> matching engine container
  -> multicast output
```

```text
tree = isolated TSIM environment / segment

inside the tree:
  - dev boxes: shared development machines
  - service box: gateway / entry point
  - controller: orchestration and control plane
  - engine: physical host running hot-path engine
  - web box: shared UI / monitoring / control interface
```

Key point: the **engine box is separate and physical**, which suggests the hot path is latency-sensitive and needs controlled hardware behavior (§8).

### Likely Proprietary / Internal Terms

- **TSIM**: likely an internal trading simulation environment or network segment.
- **Andromeda matching engine**: likely the internal engine or engine wrapper name.
- **`.ncr` files**: probably an internal or vendor-specific replay format.
- **MDF tools**: likely internal or vendor-specific market-data tooling.
- **market echo agent**: internal component for replaying, echoing, or generating market/order flow.
- **tree / dev boxes / service box / controller / engine / web box**: common role pattern, but the exact topology is internal.

Exact meanings, file formats, APIs, command syntax, and host naming are learned fastest from internal documentation or onboarding. Everything in §2–§8 is the transferable part and is worth preparing publicly.

---

## 10. Build Plan

### Highest Value

1. Build a simple deterministic matching engine.
2. Add replay from a file.
3. Emit market-data events.
4. Add sequence numbers and gap detection.
5. Add UDP multicast publisher/subscriber.
6. Profile and reduce allocation in the hot path.
7. Learn CPU pinning and NUMA basics.

### Mini-Project

```text
orders.txt
  -> replay.cpp
  -> engine.cpp
  -> market_data_publisher.cpp
  -> multicast subscriber
  -> reconstructed book
```

Start with text input. Later convert to binary.

Example input:

```text
1 ADD 1001 AAPL B 100 10
2 ADD 1002 AAPL S 101 5
3 ADD 1003 AAPL B 101 3
4 CANCEL 1001
```

Expected engine behavior:

- order `1003` crosses with sell order `1002`;
- trade is generated at resting order price;
- remaining quantities are updated;
- book update is emitted.

---

## 11. Open Questions and Gaps

**Open questions from the design sketch:**

- Ingress reassembly: do messages get scrambled in TCP?
- Backpressure: should ingress stop calling `recv` when the matcher is behind?

**Required by §1 but not yet designed:**

- Framing header, sequence numbers, and error codes.
- Structs for cancel, mass cancel, heartbeat, logout, session reject, and the expired/cancelled reports.
- Resend / snapshot requests and trade correction.
- Market-data wire messages (trades, top-3 levels, instrument directory).
- Logging, auditing, observability, shutdown, and testing/tooling.
