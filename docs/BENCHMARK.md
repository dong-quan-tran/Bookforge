# Benchmarks

## Purpose

Bookforge uses Google Benchmark to track performance regressions in the C++20 order-book core and replay path.

These results are local Release-build baselines, not universal latency claims or production-exchange guarantees. Compare results only on equivalent hardware, compiler, fixture, and build settings.

## Environment

| Property | Value |
|---|---|
| Run date | 2026-09-08 |
| Operating system | Windows development host |
| Logical CPUs reported by Google Benchmark | 20 |
| Reported CPU frequency | 2.688 GHz |
| L1 data cache | 48 KiB × 10 |
| L2 unified cache | 1,280 KiB × 10 |
| L3 unified cache | 24 MiB × 1 |
| Build mode | Release |
| Repetitions | 5 |
| Aggregate report | mean, median, standard deviation, coefficient of variation |

## Replay throughput

`benchmark_replay` loads a deterministic 10,000-event synthetic fixture once, then measures in-memory replay through:

```text
ReplayRunner
-> HyperliquidMatchingEngineAdapter
-> MatchingEngine
-> OrderBook
```

The fixture contains 8,334 `New` events and 1,666 rejected events. CSV parsing/loading occurs before the timed iterations, so this benchmark measures replay throughput rather than end-to-end file-ingestion throughput.

| Benchmark | Fixture events | Mean throughput | Median throughput | Throughput CV |
|---|---:|---:|---:|---:|
| `BM_InMemoryReplayThroughput` | 10,000 | 4.4788M events/s | 4.4843M events/s | 1.20% |

## Order-book workloads

`benchmark_order_book` exercises price-time-priority book operations. The workload benchmarks include the operations represented by their names; they are more representative for headline throughput than isolated-operation benchmarks because they process sustained batches.

| Benchmark | Workload size | Mean throughput | Median throughput | Throughput CV |
|---|---:|---:|---:|---:|
| `BM_AddOrderWorkload` | 100,000 orders | 3.2522M inserts/s | 3.2628M inserts/s | 2.93% |
| `BM_AddThenCancelWorkload` | 100,000 adds + 100,000 cancels | 5.5811M operations/s | 5.5273M operations/s | 2.63% |
| `BM_AddThenCancelWorkload` | 10,000 adds + 10,000 cancels | 5.6534M operations/s | 5.6765M operations/s | 1.82% |

The repository also includes isolated benchmarks for add, cancel, partial/full execution, quantity reduction, and requeue-at-new-price behavior across 1,000, 10,000, and 100,000 pre-populated orders.

## Reproduction

Configure a benchmark-enabled build:

```powershell
cmake -S . -B build-bench -DBOOKFORGE_ENABLE_BENCHMARKS=ON
cmake --build build-bench --config Release --target benchmark_order_book benchmark_replay
```

Run five aggregate repetitions:

```powershell
.\build-bench\bench\Release\benchmark_order_book.exe `
    --benchmark_format=console `
    --benchmark_repetitions=5 `
    --benchmark_report_aggregates_only=true

.\build-bench\bench\Release\benchmark_replay.exe `
    --benchmark_format=console `
    --benchmark_repetitions=5 `
    --benchmark_report_aggregates_only=true
```

If using a different CMake generator, locate the generated executables and adjust the path accordingly.

## Interpretation

- The replay result is an in-memory deterministic-fixture baseline, not live-market throughput.
- Event-time pacing should be disabled for throughput tests because it intentionally waits for source timestamp gaps.
- Performance is affected by compiler version, CPU topology, power settings, memory pressure, and benchmark fixture composition.
- Run benchmarks after functional changes to matching, replay, adapters, or book data structures when assessing possible regressions.