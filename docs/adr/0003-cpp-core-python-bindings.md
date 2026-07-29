# ADR 0003: Keep matching logic in C++ and expose bindings to Python

- Status: Accepted
- Date: 2026-07-29

## Context

Bookforge is intended to combine systems engineering and research workflows in one repository.

The project needed:
- a performant and testable matching-engine core,
- a convenient scripting and experimentation layer,
- and a clear boundary between performance-sensitive logic and research tooling.

A design decision was required on where core matching behavior should live.

## Decision

Bookforge keeps the authoritative matching, replay, and performance-sensitive logic in C++ and exposes selected functionality to Python through bindings.

Python is used for:
- dataset construction,
- model training,
- evaluation workflows,
- API behavior,
- and downstream experimentation.

C++ remains the source of truth for:
- order-book state transitions,
- replay mechanics,
- and benchmark-sensitive hot paths.

## Alternatives considered

### Implement matching logic directly in Python

This would simplify some scripting workflows.

This was not chosen because:
- it would weaken the systems-engineering side of the project,
- it would reduce realism for performance and benchmarking work,
- and it would duplicate logic across languages if a C++ core were still retained.

### Split independent C++ and Python implementations

This would allow language-specific experimentation.

This was not chosen because:
- it would create synchronization risk between implementations,
- it would complicate correctness guarantees,
- and it would make tests and documentation harder to trust.

## Consequences

### Positive
- The repo has a clear systems core with a usable research layer on top.
- Performance-critical behavior stays in the language best suited for it.
- Python workflows remain convenient without redefining matching semantics.
- The overall project narrative is stronger for interviews and portfolio review.

### Negative
- Bindings add integration and packaging complexity.
- Some interfaces must be designed around cross-language boundaries.
- Not every C++ feature should be exposed directly, which requires ongoing judgment.