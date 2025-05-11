# Architecture of the Market Data Dissemination Simulator

## Overview
The Market Data Dissemination Simulator is a high-performance C++23 application designed to simulate low-latency market data dissemination. It comprises four components: Market Data Generator, Dissemination Engine, Subscriber Interface, and Performance Monitor.

## Architecture Diagram
The following diagram illustrates the system’s components and dataflow:

![Architecture Diagram](architecture.png)

The Market Data Generator produces Quote and Trade structs, simulating exchange market data, and pushes them to a Boost.Lockfree queue for thread-safe, low-latency handoff to the Dissemination Engine. The Dissemination Engine broadcasts incremental updates (quotes and trades, no order book snapshots in MVP) via ZeroMQ UDP multicast to the Subscriber Interface, which manages TCP connections and delivers filtered data to subscribers (e.g., market-making or arbitrage strategies). Subscribers send subscription requests (e.g., "subscribe to AAPL") via ZeroMQ TCP to the Subscriber Interface, which forwards them to the Dissemination Engine. The Performance Monitor collects latency (primarily queue read to client receipt) and throughput metrics from the Dissemination Engine and Subscriber Interface, with optional metrics from the Generator for end-to-end analysis. Metrics are logged asynchronously using spdlog to minimize overhead.

## Design Patterns
- **Producer-Consumer**:
  - **Use**: Market Data Generator produces Quote/Trade structs, consumed by the Dissemination Engine via a Boost.Lockfree queue.
  - **Rationale**: Enables thread-safe, low-latency data transfer. 
  - **Implementation**: Generator runs in a dedicated thread, pushing to a lock-free queue with capacity for 10K messages.

- **Publish-Subscribe**:
  - **Use**: Dissemination Engine publishes incremental updates via ZeroMQ UDP multicast; Subscriber Interface filters data based on TCP subscription requests.
  - **Rationale**: Decouples publishers and subscribers, supports dynamic filtering, and aligns with market data dissemination protocols.
  - **Implementation**: ZeroMQ PUB/SUB sockets for multicast; TCP-based subscription manager using REQ/REP for subscription requests.

- **Observer**:
  - **Use**: Performance Monitor observes Dissemination Engine and Subscriber Interface for latency and throughput metrics, with optional Generator monitoring.
  - **Rationale**: Non-intrusive metric collection preserves hot-path performance; modular for adding new metrics.
  - **Implementation**: Callbacks via `IMonitor` interface; asynchronous logging with spdlog.

## Module Interactions
| Module                 | Input                          | Output                         | Interaction Details                              |
|------------------------|--------------------------------|--------------------------------|-------------------------------------------------|
| Market Data Generator  | Config (rate, symbols)        | Quote/Trade structs, optional metrics | Pushes structs to Boost.Lockfree queue; optionally sends generation rate to Monitor. |
| Dissemination Engine   | Structs from queue, subscription requests | Multicast incremental updates, latency metrics | Reads queue; broadcasts via ZeroMQ UDP; processes TCP subscription requests; sends queue read/send timestamps to Monitor. |
| Subscriber Interface   | Multicast updates, client subscriptions | Filtered updates to clients, receipt metrics | Receives multicast; forwards subscription requests via TCP; sends filtered updates to clients via TCP; sends receipt timestamps to Monitor. |
| Performance Monitor    | Metrics from Engine, Interface, optional Generator | Log file | Collects latency (dissemination-to-subscriber, optional end-to-end) and throughput; logs asynchronously. |

## Implementation Notes
- **Low-Latency**:
  - Structs are cache-aligned (64-byte boundaries) to minimize cache misses.
  - Boost.Lockfree queues ensure thread-safe, non-blocking data handoff.
  - No dynamic allocations in hot paths; fixed-size message structs.
- **Performance Monitoring**:
  - Primary focus: Dissemination-to-subscriber latency (queue read to client receipt).
  - Timestamps captured at queue read, multicast send, and client receipt using `std::chrono::high_resolution_clock`.
  - Throughput measured as messages processed per second.
  - Asynchronous logging with spdlog minimizes overhead; batch logging every 1000 messages or 1s.
  - Optional generator metrics (e.g., message rate) for testing pipeline performance.
- **Subscriptions**:
  - Clients connect to Subscriber Interface via ZeroMQ TCP SUB sockets.
  - Subscription requests (e.g., "subscribe to AAPL") forwarded to Dissemination Engine via TCP REQ/REP.
  - Subscriber Interface filters multicast messages based on subscriptions.
  - Unsubscription follows the same TCP path, stopping filtered updates.
- **Extensibility**:
  - Message structs use fixed-size fields to support new asset classes (e.g., options).
  - Networking layer allows swapping protocols (e.g., reliable multicast) in Beyond MVP.
  - Performance Monitor can add metrics (e.g., jitter) or telemetry (e.g., Prometheus).
- **Modularity**:
  - Modules implement interfaces (e.g., `IGenerator`, `IMonitor`) for testability.
  - Components are independently testable, with Google Test suites planned.