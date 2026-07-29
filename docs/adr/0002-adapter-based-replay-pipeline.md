# ADR 0002: Use an adapter-based replay pipeline

- Status: Accepted
- Date: 2026-07-29

## Context

Bookforge replays external event data into a matching-engine core.

The replay path had to support:
- CSV-driven historical or synthetic fixtures,
- deterministic iteration over event sequences,
- separation between ingestion and matching logic,
- and future extension points for metrics, simulation, or alternative replay sinks.

A design choice was needed for how replay events should be translated into engine operations.

## Decision

Bookforge uses an adapter-based replay pipeline:

`HyperliquidCsvReader -> ReplayRunner -> IReplayAdapter -> HyperliquidMatchingEngineAdapter -> MatchingEngine`

In this design:
- the CSV reader is responsible for parsing input data,
- the replay runner is responsible for sequencing and replay bounds,
- the adapter is responsible for translating replay events into engine-facing actions,
- and the matching engine remains focused on order-book behavior rather than exchange-specific input semantics.

## Alternatives considered

### Replay logic directly inside the matching engine

This would couple external replay formats and event semantics directly to the core engine.

This was not chosen because:
- it would blur the boundary between core matching logic and ingestion logic,
- it would make testing more fragile,
- and it would reduce flexibility for future replay targets.

### Replay logic directly inside the CSV reader

This would make the input layer responsible for orchestration and execution semantics.

This was not chosen because:
- parsing and execution are different responsibilities,
- it would make benchmarks and isolated tests harder,
- and it would reduce reuse across fixtures and future input sources.

## Consequences

### Positive
- The replay pipeline is modular and easier to test in pieces.
- Synthetic fixtures and historical fixtures can share the same flow.
- Future extensions such as injected orders, simulation adapters, or metrics collectors have a natural seam.
- Benchmark targets can isolate replay path behavior more cleanly.

### Negative
- The architecture introduces more types and boundaries than a single-file replay implementation.
- Some event types are currently tracked rather than fully modeled.
- Future adapter growth will need discipline to avoid becoming a catch-all layer.