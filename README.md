# Limit Order Book (LOB) Matching Engine

A low-latency limit order book matching engine written in standard C++. 

I built this to simulate the core infrastructure of a high-frequency trading exchange. The engine strictly enforces Price-Time Priority matching and is optimized to avoid O(N) memory shifting during order cancellations or fills.

## Memory Architecture

To achieve microsecond latency, the engine relies on two standard library data structures:

1. **Price Levels (`map`)** A Red-Black Tree is used to maintain price levels. Bids are sorted descending (`greater`), and Asks are sorted ascending. This guarantees O(log N) time complexity for finding the best bid/ask and checking if the spread has been crossed.

2. **Order Queues (`list`)**
   At each individual price level, orders are stored in a doubly-linked list. In a real-world scenario where a trader cancels a resting order, this allows the engine to pop that order out of the middle of the queue in O(1) time without forcing the CPU to shift a contiguous array (`vector`) in memory. 

## Performance Benchmark

The `main()` function includes a high-frequency stress test generating 100,000 random algorithmic orders (mixed buys/sells). 

**Benchmark Results (AMD Ryzen 7, Compiled with `g++ -O3`):**
- **Throughput:** ~1.69 Million orders/sec (100,000 orders in 59 ms)
- **p50 Latency:** 0.4 µs
- **p95 Latency:** 0.7 µs
- **p99 Latency:** 1.0 µs

## How to Run

Because this relies purely on the C++ Standard Template Library, there are no external dependencies or build systems required. It is contained entirely in a single `engine.cpp` file for easy reading.

Clone the repository and compile using g++ with the O3 optimization flag for accurate performance metrics:

```bash
g++ -O3 engine.cpp -o engine
./engine