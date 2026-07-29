# ADR 0001: Use a price-time-priority matching core

- Status: Accepted
- Date: 2026-07-29

## Context

Bookforge is a limit-order-book project intended to demonstrate both matching-engine correctness and market microstructure research workflows.

The core matching model needed to satisfy several goals:
- be straightforward to reason about,
- support deterministic replay,
- expose well-defined invariants for testing,
- and remain interview-friendly without relying on unnecessary framework complexity.

A central design choice was how order priority should be enforced inside the matching engine.

## Decision

Bookforge uses a price-time-priority matching core.

Priority is determined in this order:
1. better price first,
2. earlier arrival first among orders resting at the same price.

This applies to both matching behavior and resting-book queue semantics.

Replace operations are currently treated conservatively as cancel-and-reinsert semantics for queue priority, including same-price replacement.

## Alternatives considered

### Pro-rata allocation

A pro-rata model would distribute fills across resting quantity at a price level rather than preserving FIFO queue order.

This was not chosen because:
- it adds complexity without helping the current educational and research goals,
- it is less intuitive for replay and invariant reasoning,
- and it is not the best default for a general-purpose order-book portfolio project.

### Hybrid or venue-specific priority models

Venue-specific rules could include special handling for replaces, hidden liquidity, or queue-jump semantics.

This was not chosen because:
- Bookforge currently aims for a clean and explainable base model,
- historical replay support is still simplified,
- and richer venue rules would be easier to add later than to remove now.

## Consequences

### Positive
- Matching behavior is easy to explain and test.
- The book invariants are clear and stable.
- Replay and benchmarking operate on a deterministic and understandable core.
- The architecture remains aligned with interview and portfolio goals.

### Negative
- The model does not represent every venue-specific matching nuance.
- Same-price replace losing priority may be stricter than some real venues.
- More realistic queue-position experiments may require extending this decision later.