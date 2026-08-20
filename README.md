# VoltKV

A concurrent, high-performance in-memory key-value store and caching server written in modern C++17.

VoltKV is designed as a systems-programming project focusing on:

- TCP networking
- Linux `epoll`
- non-blocking sockets
- thread pools
- concurrent data structures
- `std::shared_mutex`
- per-client command ordering
- TTL-based expiration
- LRU cache eviction
- end-to-end performance benchmarking

---

## Features

### Core Key-Value Operations

VoltKV supports the following commands:

```text
SET <key> <value>
SET <key> <value> <ttl>
GET <key>
DELETE <key>
EXISTS <key>
```

Examples:

```text
SET name John
GET name

SET session abc123 10
GET session

EXISTS name
DELETE name
```

Responses:

```text
SET     → OK
GET     → value / NOT_FOUND
DELETE  → 1 / 0
EXISTS  → 1 / 0
```

---

## Architecture

VoltKV uses an event-driven networking layer combined with a worker thread pool.

```text
                         ┌─────────────────────┐
                         │       Clients       │
                         │  TCP connections    │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │   Non-blocking TCP  │
                         │       Socket        │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │        epoll        │
                         │  Event Multiplexer  │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │   Client Handling   │
                         │  + command parsing  │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │     ThreadPool      │
                         │                     │
                         │  Worker  Worker ... │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │       KVStore       │
                         │                     │
                         │ unordered_map       │
                         │ + shared_mutex      │
                         │ + TTL               │
                         │ + LRU               │
                         └─────────────────────┘
```

---

## Networking

### Non-blocking TCP

The server uses Linux TCP sockets with non-blocking mode enabled.
The listening socket and connected client sockets are configured with:

```text
O_NONBLOCK
```

This prevents the server's event loop from blocking on socket operations.

### epoll

VoltKV uses Linux `epoll` for event-driven I/O.
Instead of creating a dedicated thread for every connected client, the server maintains a single event loop that monitors many sockets.

Conceptually:

```text
                  epoll
                    │
       ┌────────────┼────────────┐
       ▼            ▼            ▼
    Client A     Client B     Client C
       │            │            │
       └────────────┼────────────┘
                    │
               ready events
```

This allows the server to efficiently manage multiple simultaneous TCP connections.

### Thread Pool

VoltKV uses a fixed-size worker thread pool.

The thread pool consists of:

- worker threads
- a shared task queue
- a mutex protecting the queue
- a condition variable
- graceful worker shutdown

Instead of creating a new thread for every request, tasks are submitted to the existing workers.

```text
                    ThreadPool
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
       Worker 1      Worker 2      Worker 3
          │             │             │
          └─────────────┼─────────────┘
                        │
                    task queue
```

This avoids repeated thread creation/destruction overhead and provides bounded concurrency.

---

## Client Concurrency and Ordering

Each connected client maintains its own state.

The state includes:

- receive buffer
- pending commands
- processing status
- connection activity status
- synchronization mutex

This allows VoltKV to handle TCP fragmentation correctly.

For example, a command may arrive as:

```text
SET foo
```

followed by:

```text
 bar\n
```

The server accumulates incoming bytes in the client's receive buffer until a complete newline-terminated command is available.

### Per-client command ordering

Commands belonging to the same client are processed in order.

For example:

```text
SET x 10
SET x 20
GET x
```

will preserve the logical command order.

At the same time, independent clients can execute concurrently:

```text
Client A ──► Worker 1
Client B ──► Worker 2
Client C ──► Worker 3
```

This provides concurrency without sacrificing per-client ordering.

---

## Thread-Safe KVStore

The underlying key-value store uses:

```text
std::unordered_map
```

for average O(1) key lookup.

Access to the store is protected using:

```text
std::shared_mutex
```

The design allows shared access where appropriate while exclusive access is used for mutations.

The store supports:

- SET
- GET
- DELETE
- EXISTS

and is safe for concurrent access from multiple worker threads.

---

## TTL — Time-To-Live

VoltKV supports optional expiration times for keys.

A normal key:

```text
SET name John
```

does not expire.

A TTL key:

```text
SET session abc123 10
```

expires after 10 seconds.

Internally, entries maintain:

- value
- expiration timestamp
- has_expiry

TTL uses:

```text
std::chrono::steady_clock
```

rather than wall-clock time.

This provides a monotonic clock suitable for measuring elapsed time.

### Lazy expiration

VoltKV uses lazy expiration.

Expiration is checked when a key is accessed:

```text
GET key
   │
   ▼
Does key exist?
   │
   ├── No ───────► NOT_FOUND
   │
   ▼
Has key expired?
   │
   ├── Yes ──────► Remove key
   │                │
   │                ▼
   │             NOT_FOUND
   │
   └── No ───────► Return value
```

This avoids maintaining a separate expiration thread.

---

## LRU Cache Eviction

VoltKV supports capacity-based LRU eviction.

The cache can be created with a configurable maximum capacity:

```cpp
KVStore store(10000);
```

The default capacity is:

```text
10000
```

When the cache exceeds its capacity, the least recently used entry is evicted.

### LRU Data Structure

LRU is implemented using:

```text
std::unordered_map
+
std::list
```

The unordered map provides average O(1) key lookup.
The list maintains usage order:

```text
MRU               LRU
 │                 │
 ▼                 ▼
[A] → [C] → [D] → [B]
```

Each entry stores an iterator pointing to its position in the LRU list.
This allows entries to be moved to the front in O(1).

### Example

Suppose the capacity is 3:

```text
SET A 1
SET B 2
SET C 3
```

The LRU order is:

```text
MRU → C → B → A ← LRU
```

Now:

```text
GET A
```

updates the order:

```text
MRU → A → C → B ← LRU
```

Then:

```text
SET D 4
```

causes:

```text
B → evicted
```

because B is the least recently used entry.

### TTL + LRU Interaction

TTL and LRU operate together.

A TTL-enabled entry participates normally in LRU ordering.

For example:

```text
SET A 1
SET B 2 30
SET C 3
```

The TTL on B does not prevent it from being evicted by LRU.

Likewise, when a TTL entry expires, it is removed from both:

- `unordered_map`
- LRU list

This keeps the two data structures consistent.

### Complexity

The main KVStore operations are designed around average O(1) operations.

| Operation    | Average Complexity|
|--------------|-------------------|
| SET          |               O(1)|
| GET          |               O(1)|
| DELETE       |               O(1)|
| EXISTS       |               O(1)|
| LRU update   |               O(1)|
| LRU eviction |               O(1)|
| TTL check    |               O(1)|

The actual performance also depends on locking, networking, thread scheduling, and system load.

---

## Command Parser

Commands are parsed from newline-terminated input.

For example:

```text
SET name John
```

becomes:

```text
["SET", "name", "John"]
```

and:

```text
SET session abc123 10
```

becomes:

```text
["SET", "session", "abc123", "10"]
```

The server validates command names and argument counts before executing operations.

Invalid commands produce:

```text
ERROR invalid command
```

Invalid TTL values produce:

```text
ERROR invalid TTL
```

---

## Building

### Requirements

VoltKV currently targets Linux/POSIX systems and uses Linux-specific networking APIs.

Requirements:

- C++17 compiler
- CMake 3.15+
- Linux / WSL
- pthread support

### Build

From the project root:

```bash
cmake -S . -B build
cmake --build build -j
```

The main server executable is:

```text
build/kvstore
```

---

## Running the Server

Start VoltKV:

```bash
./build/kvstore
```

The server listens on:

```text
127.0.0.1:6379
```

You should see:

```text
Listening on port 6379...
```

---

## Connecting to VoltKV

You can use a TCP client such as `nc`:

```bash
nc 127.0.0.1 6379
```

Then execute commands:

```text
SET name John
OK

GET name
John

EXISTS name
1

DELETE name
1

GET name
NOT_FOUND
```

TTL example:

```text
SET session abc123 10
OK

GET session
abc123
```

After approximately 10 seconds:

```text
GET session
NOT_FOUND
```

---

## Tests

VoltKV contains separate tests for the major components.

Examples include:

- KVStore unit tests
- KVStore concurrency tests
- Mixed concurrency tests
- ThreadPool tests
- ThreadPool concurrency tests
- TTL tests
- LRU tests
- Lock performance benchmark
- End-to-end server benchmark

Build all targets with:

```bash
cmake --build build -j
```

Then execute the individual test binaries from the build directory.

Example:

```bash
./build/kvstore_test
./build/kvstore_conc_test
./build/kvstore_mixed_conc_test
./build/threadpool_test
./build/threadpool_concurrency_test
./build/kvstore_ttl_test
./build/kvstore_lru_test
```

---

## Performance Benchmark

VoltKV includes an end-to-end TCP benchmark.

The benchmark creates multiple concurrent TCP clients and measures the complete request path:

```text
Benchmark client
      │
      ▼
     TCP
      │
      ▼
    epoll
      │
      ▼
 ThreadPool
      │
      ▼
   KVStore
      │
      ▼
  response
      │
      ▼
Benchmark client
```

This is different from a KVStore-only benchmark because it measures the actual server architecture.

### Benchmark Configuration

The benchmark currently uses:

```text
Operations per client: 10,000
Client counts:         1, 2, 4, 8, 16
Workload:              90% GET / 10% SET
```

The GET/SET ratio is configurable through:

```cpp
constexpr int GET_PERCENT = 90;
```

For example:

```cpp
constexpr int GET_PERCENT = 80;
```

produces an 80% GET / 20% SET workload.

### Running the Benchmark

Start the server first:

```bash
./build/kvstore
```

Then, in another terminal:

```bash
./build/kvstore_server_benchmark
```

### Benchmark Results

The following measurements were obtained using the included end-to-end benchmark.

**90% GET / 10% SET**

| Clients | Throughput (ops/sec) |
|---------|-----------------------|
| 1       | 17,470                |
| 2       | 23,856                |
| 4       | 33,167                |
| 8       | 40,861                |
| 16      | 48,975                |

**80% GET / 20% SET**

| Clients | Throughput (ops/sec) |
|---------|-----------------------|
| 1       | 18,699                |
| 2       | 24,307                |
| 4       | 33,126                |
| 8       | 37,453                |
| 16      | 59,913                |

These are localhost measurements from the development environment, not universal hardware-independent performance guarantees.

The benchmark demonstrates increasing throughput with greater client concurrency and provides a reproducible way to compare future changes.

### Lock Performance Benchmark

VoltKV also includes a benchmark comparing:

- `std::mutex`

against:

- `std::shared_mutex`

under different workloads.

Tested workloads include:

- 90% GET / 10% SET
- 50% GET / 50% SET
- 10% GET / 90% SET

and multiple thread counts.

The benchmark is intended to evaluate locking behavior rather than overall network throughput.

---

## Project Structure

A typical project layout is:

```text
VoltKV/
│
├── include/
│   ├── CmdParser.h
│   ├── KVStore.h
│   ├── Server.h
│   └── ThreadPool.h
│
├── src/
│   ├── CmdParser.cpp
│   ├── KVStore.cpp
│   ├── Server.cpp
│   ├── ThreadPool.cpp
│   └── main.cpp
│
├── tests/
│   ├── KVStoreTest.cpp
│   ├── KVStoreConcurrencyTest.cpp
│   ├── KVStoreMixedConcurrencyTest.cpp
│   ├── KVStoreTTLTest.cpp
│   ├── KVStoreLRUTest.cpp
│   ├── ThreadPoolTest.cpp
│   └── ThreadPoolConcurrencyTest.cpp
│
├── benchmarks/
│   ├── LockBenchmark.cpp
│   └── ServerBenchmark.cpp
│
├── CMakeLists.txt
└── README.md
```

---

## Design Decisions

### Why epoll?

A thread-per-client architecture becomes expensive as the number of simultaneous connections grows.

`epoll` allows a single event loop to efficiently monitor many file descriptors and react only when they become ready.

### Why a ThreadPool?

Creating a new thread for every operation introduces unnecessary thread creation and scheduling overhead.

A fixed-size worker pool provides:

- bounded concurrency
- reusable worker threads
- centralized task management
- lower thread creation overhead

### Why shared_mutex?

The KV store has a mixture of reads and writes.

A `shared_mutex` provides shared locking semantics for operations that can safely run concurrently while still providing exclusive locking for mutations.

With LRU enabled, operations such as GET can modify the LRU ordering, so they require exclusive synchronization as well.

### Why unordered_map?

The primary KV lookup needs to be fast.

`std::unordered_map` provides average O(1) lookup, insertion, and deletion.

### Why list for LRU?

An LRU cache needs to move accessed entries to the MRU position efficiently.

`std::list::splice()` allows an existing node to be moved in O(1).

Combining:

- `unordered_map`
- `list`

provides efficient lookup and efficient recency updates.

### Why steady_clock for TTL?

TTL is based on elapsed time.

`std::chrono::steady_clock` is monotonic and therefore appropriate for measuring durations without being affected by wall-clock adjustments.

---

## Concurrency Model

VoltKV separates networking from request execution.

```text
                Main / epoll thread
                        │
                  socket events
                        │
                        ▼
                 ClientState
                        │
                  command queue
                        │
                        ▼
                  ThreadPool
                 ┌──────┼──────┐
                 ▼      ▼      ▼
              Worker  Worker  Worker
                 │      │      │
                 └──────┼──────┘
                        ▼
                     KVStore
```

This design allows different clients to make progress concurrently while preserving command ordering within each client.

---

## Error Handling

The server handles common socket errors including:

- socket creation failure
- bind failure
- listen failure
- accept failure
- receive errors
- send errors
- client disconnects
- epoll registration errors

Invalid application commands are rejected before reaching the KV store.

---

## Current Scope

VoltKV is currently an:

- in-memory
- single-node
- TCP-based
- concurrent
- event-driven
- TTL-enabled
- LRU-enabled

key-value cache/server.

It is designed primarily as a systems-programming and performance-engineering project.

---

## Future Improvements

Possible future extensions include:

- Write-Ahead Logging (WAL)
- persistent storage
- snapshotting
- replication
- sharding
- clustering
- richer wire protocols
- pipelined commands
- asynchronous/non-blocking response handling
- more detailed latency measurements
- percentile latency reporting such as p50/p95/p99
- additional cache replacement policies

These are intentionally outside the current implementation scope.

---

## Technologies

| Category         | Technology                      |
|------------------|---------------------------------|
| Language         | C++17                           |
| Build System     | CMake                           |
| Networking       | Linux TCP sockets               |
| I/O Multiplex    | epoll                           |
| Concurrency      | std::thread                     |
| Synchronization  | std::mutex / std::shared_mutex  |
| Containers       | unordered_map / list / deque    |
| Timing           | std::chrono                     |
| Platform         | Linux / WSL                     |

---

## Author

**Satyaki Ray**

VoltKV was developed as a systems-programming project focused on concurrency, networking, caching, and performance engineering.