# Market Data Dissemination Simulator Requirements

## 1. Overview
The Market Data Dissemination Simulator is a high-performance C++ application designed to simulate the generation, dissemination, and consumption of market data in a low-latency environment. 

## 2. Functional Requirements

### 2.1 Market Data Generation
- Generate synthetic market data for trades & quotes (bids/asks)
- Support data types: quotes (bid/ask prices and sizes) and trades (price, volume, timestamp).
- Ensure data consistency: Valid price/volume ranges, no negative values.

### 2.2 Data Dissemination
- Distribute market data to subscribers with minimal latency. 
- Use UDP multicast for mvp (can later extend to support reliable multicast).

### 2.3 Subscriber Interface
- Provide a TCP-based server interface for subscribing to market data.
- Support message filtering by instrument or data type (e.g., quotes only, specific symbols).
- Allow subscriptions to specific instruments or data types.
- Support 100 concurrent subscribers with consistent latency.

### 2.4 Subscriber
- TCP client that subscribes to the subcriber interface.
- Asks for selection of symbols or datatypes and is sent those from the interface.
- The idea is that this represents some trading strategy.
- Would also like to evaluate switching the TCP connection to a TCP+UDP connection with TCP for managing the subscription and UDP for the data.

### 2.5 Performance Monitoring
- Measure end-to-end latency
- Measure latency of disseminator -> subscriber interface
- Measure latency of subscriber interface -> subscriber
- Track throughput (messages/sec) and latency variance.
- Log metrics to a file with minimal performance impact (using spdlog or similar).


## 3. Non-Functional Requirements

### 3.1 Performance
- **Latency**: Achieve <10µs average latency. But should be as low as possible ideally. 
- **Throughput**: Handle 100K messages/sec. See impact of increasing this once have something working.
- **Scalability**: First make work for single subscriber interface. Then scaling to 100+ subscribers would be nice
- **Reliability**: No crashes or data loss under normal load; graceful degradation under extreme load.

### 3.2 Technical Constraints
- **Language**: C++23 
- **Platform**: Windows 11 for development and testing. {unfortunatly}
- **Dependencies**: Minimize external libraries; use ZeroMQ (networking), Boost.Lockfree (queues), spdlog (logging), and Google Test (testing)
- **Build System**: CMake & Vcpkg

### 3.3 Design Principles
- **Low Latency**: Use lock-free data structures, cache-friendly memory layouts, and avoid dynamic allocations in hot paths.
  - Note: Lockfree does not mean faster, ideally I would like to try out different queue types.
- **Modularity**: Design components (generator, disseminator, subscriber) to be independently testable and replaceable.
- **Extensibility**: Allow easy addition of new data types or protocols.
- **Maintainability**: Include clear documentation and comprehensive unit tests.

## 4. Edge Cases and Fault Tolerance
- Handle packet loss in dissemination (logging first, later can implement retransmission too).
- Manage subscriber failures (e.g., dropped connections) without impacting other subscribers.
- Later, support bursty traffic (e.g., 10x normal rate for short periods) without crashes.

## 5. Possible extensions to the MVP:
- **Advanced Low-Latency Techniques**:
  - Implement kernel bypass for network I/O, although I am not sure I can do this on my machine.
- **Reliable Multicast**: Add retransmission for lost packets to ensure data integrity under high load.
- **Stress Testing**: Simulate extreme market conditions (e.g., flash crashes, 10M messages/sec bursts) and benchmark. Also would like to compare to industry standards.

## 6. Success Criteria
Goal of this project is learning.
- **MVP**: Functional simulator handling 100K messages/sec with <10µs latency, supporting 100 subscribers.
