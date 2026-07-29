# Architecture Decision Records

This directory contains Architecture Decision Records (ADRs) for Bookforge.

An ADR captures one important architectural decision, why it was made, what alternatives were considered, and what consequences follow from that decision.

## Format

Each ADR is a standalone Markdown file with:
- a monotonic numeric prefix,
- a short descriptive filename,
- status,
- context,
- decision,
- alternatives considered,
- consequences.

## Current ADRs

- `0001-price-time-priority-core.md`
- `0002-adapter-based-replay-pipeline.md`
- `0003-cpp-core-python-bindings.md`

## Notes

- ADRs should be append-only in spirit.
- If a decision changes later, add a new ADR that supersedes the older one instead of rewriting history.
- ADRs are intended to explain architectural intent, not replace implementation docs.