# Benchmarks

## Purpose

This document records the current benchmark coverage in Bookforge and provides a reproducible baseline for future performance comparisons.

The goal is not to claim universal performance numbers. The goal is to:
- track regressions,
- keep benchmark scope explicit,
- and make current results easy to discuss during review or interviews.

## Benchmark targets

Bookforge currently includes two benchmark executables:

- `benchmark_order_book`
- `benchmark_replay`

These serve different purposes:
- `benchmark_order_book` measures isolated hot-path operations in the core order book and matching engine.
- `benchmark_replay` measures end-to-end replay throughput across CSV loading, replay iteration, adapter translation, and matching-engine submission.

## Current benchmark coverage

### `benchmark_order_book`

This benchmark covers the following operations:

- `AddOrder`
- `CancelOrder`
- `ExecuteTopOrderPartial`
- `ExecuteTopOrderFull`
- `ReduceOrderQuantity`
- `ReplaceOrderSamePrice`
- `ReplaceOrderNewPrice`

This target is intended to detect regressions in the most performance-sensitive core order-book paths.

### `benchmark_replay`

This benchmark covers replay throughput over a loaded fixture using the replay pipeline:

`HyperliquidCsvReader -> ReplayRunner -> IReplayAdapter -> HyperliquidMatchingEngineAdapter -> MatchingEngine`

This target is intended as an end-to-end replay benchmark rather than an isolated microbenchmark.

## Current baseline results

### Replay throughput baseline

The current replay benchmark uses a larger synthetic fixture so that measured throughput is dominated less by benchmark harness overhead and more by actual replay work.

| Benchmark | Fixture | Build | Observed result |
|---|---|---|---|
| `benchmark_replay` | Large synthetic CSV fixture | Release | roughly 4.4M–4.7M events/sec on Windows |

These numbers should be treated as local baseline measurements, not as universal performance claims.

### Order book benchmark baseline

The order book benchmark is currently used mainly as an operation-level regression check.

| Benchmark | Scope | Current interpretation |
|---|---|---|
| `benchmark_order_book` | Hot-path order-book operations | Stable and believable microbenchmark coverage for core operations |

If desired, this section can later be expanded with per-operation tables copied directly from benchmark output.

## Reproducing results

### Build benchmarks

#### Windows PowerShell
```powershell
cmake --build build --config Release --parallel
```

### Run order book benchmark

#### Windows PowerShell
```powershell
.\build\bench\Release\benchmark_order_book.exe
```

### Run replay benchmark

#### Windows PowerShell
```powershell
.\build\bench\Release\benchmark_replay.exe
```

## Fixture generation

The replay benchmark depends on a large replay-compatible fixture.

Generate or refresh the current large fixture with:

#### Windows PowerShell
```powershell
python tools\generate_replay_fixture.py --events 10000 --base-price 100.00 --output tests\fixtures\hyperliquid_replay_fixture_large.csv
```

## Notes on interpretation

Benchmark results in this repo should be read with a few constraints in mind:

- Local machine, compiler, and build settings affect absolute numbers.
- The replay benchmark measures the current adapter behavior, and today real matching work is concentrated in `EventType::New`.
- The replay benchmark is most useful as a relative baseline for future changes.
- The order book benchmark is more useful for hot-path regression tracking than for external headline numbers.

## When to update this document

Update this file when:
- benchmark scope changes,
- replay fixtures materially change,
- benchmark executables are renamed,
- or new baseline numbers become worth recording.

Avoid updating it for tiny run-to-run fluctuations.