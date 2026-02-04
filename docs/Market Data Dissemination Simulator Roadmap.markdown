# Market Data Dissemination Simulator Roadmap

This roadmap outlines the development of a high-performance Market Data Dissemination Simulator in C++. The project is divided into three stages: Architectural Design, Minimum Viable Product (mvp), and possible extensions.
## Stage 1: Architectural Design
Objective: Define a robust, scalable, and low-latency architecture.

### Tasks:
1. **Requirements Analysis**:
   - Define core functionality: Simulate market data generation, dissemination, and consumption.
   - Support high-throughput market data (e.g., 1M+ messages/sec) with minimal latency.
   - Handle multiple datatypes limited to quotes and trades.

2. **System Components**:
   - **Market Data Generator**: Produces synthetic market data (quotes and trades) with configurable rate. Later we can extend this to introduce different generation patterns (e.g, busts).
   - **Dissemination Engine**: Distributes data to subscriber interfaces with minimal latency using UDP multicast.
   - **Subscriber Interface**: Allows clients to subscribe to specific instruments or data types. Acts as a TCP server for those clients.
   - **Subscriber**: Represents some trading strategy. Subscribers to the Subscriber interface and requests a specific subset of data. Use TCP to begin with, later see if we can do TCP for control pane and UDP for data pane.
   - **Performance Monitor**: Tracks latency, throughput, and jitter with nanosecond precision.

3. **Technology Choices**:
   - **Language**: C++23 
   - **Networking**: ZeroMQ 
   - **Data Structures**: Lock-free queues (that is Boost.Lockfree) for inter-thread communication. Later investigate different how queue type affects performance.
   - **Logging**: spdlog
   - **Testing**: Google Test for unit & integration tests as well as custom stress tests for performance.

4. **Design Principles**:
   - **Low Latency**: Minimize context switches, use cache-friendly data structures, and avoid dynamic allocations in hot paths.
   - **Modularity**: Design components to be independently testable and replaceable.
   - **Extensibility**: Allow easy addition of new data types or protocols.

5. **Deliverables**:
   - Architecture document with component diagrams and data flow.
   - Initial project setup (CMake, dependencies, CI/CD pipeline).

---

## Stage 2: Minimum Viable Product (MVP)
Objective: Build a functional prototype demonstrating core low-latency dissemination with measurable performance.

### Tasks:
1. **Core Implementation**:
   - **Market Data Generator**:
     - Generate synthetic quote and trade messages for a single asset class (e.g., equities).
     - Generate random numbers for now. We can do a random walk or something more realistic in the future.
   - **Dissemination Engine**:
     - Implement UDP-based multicast for data distribution.
     - Use a single-threaded event loop for simplicity.
   - **Subscriber Interface**:
     - Basic TCP-based server to subscribe and forward data.
     - Manages multiple TCP connections at once.
     - Simple filtering by instrument or message type.
   - **Subscriber**
     - Simple TCP client to connect to subscriber interface.
     - Selects some symbol and requests that from the subscriber interface to provide a feed from.
     - Later, would be interesting to switch to TCP + UDP here too.
   - **Performance Monitor**:
     - Measure end-to-end latency.
     - Log basic metrics (e.g., average latency, throughput).

2. **Optimizations**:
   - Use UDP for data and TCP for control pane where TCP is used before. Measure performance difference.
   - Pin threads to cores to reduce context switching.
   - Optimize memory layout for cache efficiency (e.g., struct padding, alignment).

3. **Testing**:
   - Unit tests for each component (e.g., data generator produces valid messages).
   - Integration test for Disseminator - Subscriber interface.
   - Integration test for subscriber interface and subscriber.
   - Stress test with 100K messages/sec to measure latency and throughput.
   - Validate no packet loss in dissemination under load.

4. **Deliverables**:
   - Working MVP with a single asset class and basic dissemination.
   - Performance report with latency and throughput metrics (e.g., 99th percentile latency < 10µs).
   - Source code with documentation and tests.
   - Demo showing data generation, dissemination, and subscription.

---

## Stage 3: Beyond MVP
Objective: Enhance the simulator to more realistic application, focusing on advanced low-latency techniques and scalability.

### Tasks:
1. **Feature Enhancements**:
   - **Market Data Generator**:
     - Support realistic market dynamics (e.g., bursts of traffic).
     - Add bursty traffic patterns to simulate market events.
   - **Dissemination Engine**:
     - Implement reliable multicast with retransmission for lost packets.
   - **Subscriber Interface**:
     - Add advanced filtering (e.g., by price range, volume).
   - Exchange TCP with TCP+UDP wherever possible.

2. **Advanced Optimizations**:
   - Use custom memory allocators to eliminate allocation latency in hot paths.
   - Implement kernel bypass for network I/O.
   - Optimize for cache coherence with data-oriented design.
   - Implement cache warming.

3. **Performance Monitoring**:
   - Measure jitter and tail latency under extreme loads (e.g., 5M messages/sec).

4. **Testing and Validation**:
   - Simulate real-world scenarios (e.g., market open/close).
   - Benchmark against industry standards.
   - Add fault injection tests (e.g., packet loss, subscriber crashes).
   - Profile with tools like perf. 


---

## Success Criteria
- Well-documented, modular, and maintainable project.
- Build incremental understanding
- Do modular testing 
