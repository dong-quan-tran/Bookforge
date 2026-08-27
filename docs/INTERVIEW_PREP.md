# Interview Prep - Bookforge

## Project overview

Bookforge is a hybrid C++ + Python market microstructure project centered on a replayable limit-order-book core. The repo combines a low-latency C++20 matching engine, deterministic replay, snapshot export, feature export, Python-based research workflows, benchmarks, and a lightweight API/dashboard layer.

## Core questions

### Why C++ for the order book?
C++ is used because the order book sits on the hot path, where control over memory layout, branching, allocations, and cache behavior matters most. It also makes it easier to reason about latency and to keep the engine deterministic under replay.

### Why `std::map` first?
`std::map` gives a clean correctness-first implementation with sorted traversal, which is ideal while validating matching logic and invariants. It is easy to understand, easy to test, and a good baseline before introducing more specialized price-indexed structures.

### What is Order Flow Imbalance?
Order Flow Imbalance, or OFI, measures net buying versus selling pressure by tracking changes in bid and ask depth over time. It is useful because it captures short-horizon microstructure pressure rather than just static book state.

### What is Kyle's Lambda?
Kyle's Lambda is a price-impact coefficient that estimates how much price moves for a given amount of signed order flow. A larger lambda means the market is more sensitive to aggressive flow and less liquid.

### How do you avoid lookahead bias?
Only use information that would have been known at time \(t\) when building features. Labels must be generated strictly from future events after the feature row is formed, so the feature window and label window never overlap incorrectly.

## System design

### What does the replay pipeline do?
The replay pipeline turns CSV event data into a deterministic stream of external order events, then feeds them through the replay runner and adapter layer into the matching engine. This makes it possible to test the engine, profile throughput, and reproduce historical behavior.

### Why separate replay runner and adapter?
The replay runner handles iteration, offsets, limits, and logging, while the adapter translates external events into matching-engine actions. That separation keeps replay mechanics independent from market-specific parsing and matching behavior.

### Why keep snapshot export separate from replay?
Snapshots are useful for validation, debugging, and reproducibility, but they are a different concern from event ingestion. Separating them keeps the engine easier to test and the output easier to compare across runs.

## Performance talking points

### What benchmarks exist?
The repo includes a microbenchmark for order-book hot paths and a replay throughput benchmark for end-to-end event processing. The replay benchmark now uses a larger synthetic CSV fixture so the result reflects real work rather than mostly benchmark overhead.

### What changed recently?
The replay fixture was expanded, the benchmark path was wired cleanly through CMake, and the README/docs were updated to reflect the current project state. The benchmark now reports meaningful throughput in Release mode.

### How should benchmark results be described?
Order-book benchmark numbers should be framed as hot-path microbenchmarks, while replay benchmark numbers should be framed as end-to-end throughput. That distinction matters because the replay benchmark includes parsing, replay control, and adapter work, not just matching logic.

## Microstructure concepts

### Why does price-time priority matter?
Price-time priority is the standard matching rule in many limit-order-book systems. Better prices fill first, and among equal prices, earlier orders keep priority.

### What is FIFO at a price level?
FIFO means orders at the same price are matched in the order they arrived. This is important because it preserves deterministic and fair matching behavior within each level.

### What is a spread?
The spread is the difference between the best ask and best bid. It is one of the most basic measures of liquidity and execution cost.

### What is mid-price?
Mid-price is the average of the best bid and best ask. It is often used as a simple reference price for features and labels.

## Engineering trade-offs

### Why not optimize everything immediately?
The project first needs a correct, testable baseline before deeper optimization makes sense. Premature optimization can make the engine harder to reason about and can hide bugs in the matching logic.

### Why not start with a custom tree or array book?
A specialized book structure can be faster, but it also increases implementation complexity. Starting with `std::map` makes it easier to prove correctness before changing the data structure.

### Why is determinism important?
Determinism makes testing, debugging, and regression checking much easier. If the same input replay produces the same output, it becomes much simpler to isolate behavioral changes.

## Interview-ready summary

Bookforge is a good interview project because it demonstrates systems thinking, domain understanding, and disciplined engineering. The strongest story is that the project started with a correctness-first C++ matching engine, added deterministic replay and benchmarks, then layered on Python research and documentation to make the system useful beyond raw matching logic.

A concise way to describe it:

> Built a C++20 limit-order-book engine with price-time-priority matching, deterministic replay, snapshot/feature export, and benchmarks, then added Python tooling for research workflows and a dashboard for inspection.

## Good follow-up questions

- Why was `std::map` enough for the first version?
- What invariants does the order book maintain?
- How does the replay adapter map external events into engine actions?
- Why is the replay benchmark now meaningful?
- What would be the next optimization after correctness is locked in?
- How would you extend the research pipeline with better labels or features?

## One-minute pitch

Bookforge is a C++20 and Python market microstructure project built around a deterministic limit-order-book core. The engine uses price-time-priority matching, replayable CSV event ingestion, snapshot and feature export, and benchmarks for both hot paths and end-to-end replay. The goal is to show that the same repo can support low-latency systems work, reproducible testing, and downstream research workflows without losing clarity or correctness.