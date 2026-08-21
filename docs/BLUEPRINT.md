# Bookforge Blueprint

## Project summary

Bookforge is a hybrid **C++20 + Python** market microstructure project centered on a low-latency limit order book, historical market replay, feature extraction, and short-horizon signal research.

The long-term goal is to build a repo that demonstrates strength across:

- **systems engineering**
- **market microstructure modeling**
- **historical replay infrastructure**
- **feature engineering + ML research**
- **clear documentation, testing, and interview readiness**

This is not just an order book project. It is gradually becoming a small **market replay and research platform**.

---

## North-star objectives

- Build a clean **price-time priority limit order book** in C++.
- Support **historical replay** from external market data sources.
- Create an adapter boundary between **exchange-specific data** and **internal engine actions**.
- Export microstructure features for downstream research.
- Build a Python layer for liquidity estimation and short-horizon prediction.
- Provide a lightweight demo surface through an API and dashboard.
- Make the codebase polished enough for strong discussion in **quant SWE / quant research interviews**.

---

## Architecture pillars

### 1. Core engine
The core engine is responsible for:
- order representation
- price levels
- price-time priority
- insertion, cancellation, execution
- best bid / ask access
- spread and mid-price queries
- deterministic internal state transitions

### 2. Replay and data adapters
The replay layer is responsible for:
- loading historical datasets
- normalizing raw exchange fields
- mapping external events into internal engine-facing actions
- replaying events in deterministic order
- optionally validating replay results against reference snapshots

### 3. Feature extraction
The feature layer is responsible for:
- spread and mid-price tracking
- top-of-book and multi-level depth metrics
- order flow imbalance
- depth imbalance
- rolling liquidity measures
- feature export for Python workflows

### 4. Research layer
The research layer is responsible for:
- data cleaning and labeling
- liquidity estimation
- ML training and validation
- explainability and error analysis
- experiment logging and reproducibility

### 5. Demo and product surface
The demo layer is responsible for:
- exposing replay / feature outputs via API
- visualizing book state and metrics
- showing research outputs in a simple dashboard
- making the repo easier to understand and demo live

---

## Current direction

The original plan focused mainly on **LOBSTER-style replay**. The project now also includes an early **Hyperliquid historical replay path**, which means the architecture should support more than one data source.

This changes the design emphasis:

- not just “load one dataset,”
- but “define a reusable replay abstraction that can support multiple providers.”

Near-term design principle:
- **raw dataset -> normalization -> external event model -> engine adapter -> internal engine**

---

## Phase 1 — Core engine foundation

### Goals
Build a correct and testable C++ order book core.

### Tasks
- [x] Finalize `Order` representation in `src/core/`
- [x] Finalize `PriceLevel`
- [x] Finalize `OrderBook`
- [x] Implement add / cancel / execute paths
- [x] Implement best bid / best ask
- [x] Implement mid-price / spread helpers
- [x] Define invariants for price-time priority
- [x] Add explicit handling for empty book states
- [x] Add clear ownership and lifetime rules for orders
- [x] Document data structures in `docs/ARCHITECTURE.md`

### Testing / done criteria
- [x] 20+ focused Google Tests
- [x] Tests for FIFO behavior within a price level
- [x] Tests for bid/ask ordering correctness
- [x] Tests for partial fills
- [x] Tests for cancellations
- [x] Tests for empty-book edge cases
- [x] Basic benchmark for insert / cancel / execute latency

---

## Phase 2 — Replay foundation

### Goals
Build a deterministic replay pipeline that can ingest historical market data and turn it into engine-facing events.

### Tasks
- [x] Define a stable external event model
- [x] Define replay interfaces in `src/replay/`
- [x] Implement CSV / flat-file replay reader
- [x] Implement deterministic event ordering guarantees
- [x] Add replay statistics and instrumentation
- [x] Add replay logging for debugging
- [x] Define replay error handling for malformed rows
- [x] Add support for bounded sample replays for tests
- [x] Define replay configuration object (path, symbol, limits, filters)

### Data-source tasks
- [x] Support Hyperliquid-style enriched event replay
- [x] Document source-specific field mappings
- [x] Keep raw-provider logic out of the core engine

### Testing / done criteria
- [x] Replay a small historical sample end-to-end
- [x] Verify event counts and ordering
- [x] Add parser tests for malformed / missing data
- [x] Add fixture-based replay regression tests

## Phase 3 — Engine adapter layer

### Goals
Create a clean boundary between external market data and internal order-book actions.

### Tasks
- [x] Define `ExternalOrderEvent`
- [x] Define engine adapter interface
- [x] Map `New` events into internal passive order submissions
- [x] Decide how `Cancel` should work when external IDs are missing
- [x] Decide how `Fill` should map into internal execution state
- [x] Decide how to treat `Reject`, `Trigger`, and `Other`
- [x] Add synthetic-ID handling where necessary for partial prototypes
- [x] Keep exchange-specific status logic isolated in adapter code
- [x] Add adapter metrics: submitted, ignored, rejected, unsupported

### Design questions
- [x] Should replay drive the actual matching engine directly, or an intermediate simulation layer?
- [x] How should incomplete external data be represented without polluting core engine semantics?
- [x] What is the minimal internal API the replay path needs?

### Testing / done criteria
- [x] Adapter unit tests for each event type
- [x] Stub-backed replay integration test
- [x] One real-engine integration path using adapter calls

---

## Phase 4 — Matching engine integration

### Goals
Connect replayed events to a realistic matching workflow.

### Tasks
- [x] Implement or refine `MatchingEngine`
- [x] Define interaction between `OrderBook` and `MatchingEngine`
- [x] Support passive order insertion
- [x] Support aggressive execution path
- [x] Support partial fill bookkeeping
- [x] Support cancellation path
- [x] Emit trade / fill records
- [x] Add internal event logging hooks
- [x] Define whether snapshots are derived live or serialized after replay

### Validation tasks
- [x] Verify top-of-book evolution during replay
- [x] Compare replayed state against reference snapshots when available
- [x] Document known gaps between prototype replay and full market reconstruction

### Testing / done criteria
- [x] End-to-end replay through the real engine
- [x] Trade generation tests
- [x] Partial fill tests
- [x] Cancel-after-partial-fill tests
- [x] Replay consistency tests

---

## Phase 5 — Snapshot and serialization layer

### Goals
Make book state exportable, comparable, and reproducible across replay checkpoints.

### Tasks
- [x] Define snapshot schema
- [x] Implement `SnapshotSerializer`
- [x] Export top-N book depth
- [x] Export best bid / ask, spread, mid-price
- [x] Export replay timestamps and event counters
- [x] Support CSV snapshot output
- [x] Support binary snapshot output
- [x] Add snapshot comparison tools
- [x] Add snapshot builder from live engine/book state
- [x] Wire snapshot sources into `bookforge_core`
- [x] Add `test_snapshot` target in CMake
- [x] Fix snapshot include/build issues on Windows
- [x] Verify `test_snapshot` is discovered by CTest

### Testing / done criteria
- [x] Snapshot round-trip tests
- [x] Snapshot schema documentation
- [x] Validation against replay checkpoints
- [x] Builder unit tests for best bid / ask / spread / mid-price
- [x] Top-N depth export tests
- [x] Comparator mismatch reporting tests
- [x] CSV shape / header stability tests
- [x] Add `SnapshotDeserializer` for CSV round-trip support
- [x] Add CSV header validation
- [x] Full Phase 5 test pass in CTest
- [x] No `_NOT_BUILT` snapshot placeholder in CTest output


---

## Phase 6 — Feature extraction

### Goals
Turn replayed book state into usable microstructure features.

### Tasks
- [x] Compute best bid / ask and spread over time
- [x] Compute mid-price over time
- [x] Compute top-level depth imbalance
- [x] Compute multi-level depth imbalance
- [x] Compute OFI for levels 1–5
- [x] Add weighted OFI across levels 1–5
- [x] Add rolling liquidity / volatility context features
- [x] Define feature export format
- [x] Add feature naming and schema conventions

### Testing / done criteria
- [x] Feature unit tests
- [x] Small fixture-based expected-value tests
- [x] Feature export validated in Python

---

## Phase 7 — Python bridge and research layer

### Goals
Expose C++ outputs to Python and build the research workflow.

### Tasks
- [x] Add pybind11 bindings for core book / replay / feature objects
- [x] Build Python wrapper package
- [x] Implement feature loading utilities
- [x] Implement label generation for short-horizon prediction
- [x] Implement Kyle’s Lambda estimation
- [x] Add feature export CLI
- [x] Build first training dataset
- [x] Train XGBoost baseline
- [x] Add walk-forward validation
- [x] Add feature importance / SHAP analysis
- [x] Add ML experiment tracking

### Testing / done criteria
- [x] Pytest coverage for Python wrappers
- [x] Reproducible training run
- [x] Baseline metrics logged and documented

---

## Phase 8 — API and dashboard

### Goals
Create a lightweight surface for inspection and demos.

### Tasks
- [x] Build FastAPI service
- [x] Add endpoint for replay summary
- [x] Add endpoint for feature export / sample retrieval
- [x] Add endpoint for book snapshot inspection
- [x] Build React dashboard
- [x] Visualize spread, mid-price, depth, and imbalance
- [x] Add replay summary cards and charts
- [x] Add API tests
- [x] Add Docker setup

### Testing / done criteria
- [x] End-to-end local demo
- [x] Basic API contract tests
- [x] Dashboard loads sample replay outputs correctly

---

## Phase 9 — Performance and engineering polish

### Goals
Raise the repo from “works” to a serious, maintainable project.

### Tasks
- [x] Reach 50+ Google Tests.
- [x] Reach 50+ Pytest cases.
- [x] Add C++ benchmarks.
- [x] Add replay throughput benchmark.
- [x] Add profiling notes.
- [x] Add GitHub Actions CI.
- [x] Add formatting / linting rules.
- [x] Improve README and docs.
- [x] Finish `docs/INTERVIEW_PREP.md`.
- [x] Add architecture diagrams.
- [ ] Tag a clean milestone release.

### Current status
- [x] `benchmark_order_book` is built and running.
- [x] `benchmark_replay` is built and running.
- [x] Replay benchmark fixture was expanded from a tiny smoke dataset to a large synthetic CSV fixture.
- [x] Replay benchmark now produces meaningful throughput measurements in Release mode.
- [x] Replay benchmark fixture selection is wired through CMake instead of being hardcoded in `bench/CMakeLists.txt`.
- [x] A fixture generator script exists at `tools/generate_replay_fixture.py`.
- [x] CI formatting mismatch resolved with `clang-format`.
- [x] Merge conflict resolved and history cleaned up into separate commits.
- [x] README updated to reflect benchmark coverage and current project scope.
- [x] `docs/INTERVIEW_PREP.md` still needs to be finished.
- [x] Architecture diagrams still need to be added.
- [x] Milestone release still needs to be tagged.

---

## Phase 10 — Stretch goals

These are optional but high-value if time allows.

### Research / simulation

- [x] Add synthetic market event generator
- [ ] Add agent-based simulation mode
- [x] Allow user-injected orders during historical replay
- [x] Compare passive vs aggressive strategy behavior under replay
- [ ] Add queue-position-aware experiments if richer data becomes available

### Systems / performance

- [ ] Add lock-free replay queue
- [x] Add deterministic replay pacing
- [x] Add latency histograms
- [ ] Add multi-symbol support
- [ ] Add binary historical data reader for higher throughput

### Product / presentation

- [x] Add a polished demo scenario for interviews
- [x] Add benchmark tables to docs
- [x] Add architecture decision records (ADRs)

### Current status

- [x] Added a synthetic market-event generator CLI.
- [x] Generator emits replay-compatible CSV fixtures with deterministic seeding.
- [x] Output can be used for replay benchmarks, demos, feature-export workflows, and future strategy experiments.
- [x] Historical replay supports scheduled injected orders through replay-side abstractions.
- [x] Added `InjectedOrder`, `InjectedOrderSchedule`, and replay coverage for injected-order dispatch.
- [x] Replay adapter interface supports both external events and injected orders.
- [x] Added a strategy-experiment configuration model for passive/aggressive mode, side, entry offset, limit price, quantity, and injection timing.
- [x] Added injected-order construction and single-order scheduling helpers for strategy experiments.
- [x] Added `StrategyExperimentAdapter` and `StrategyExperimentRunner` scaffolding for single-trial replay experiments.
- [x] Added a strategy-experiment CLI with configuration parsing for input CSV, output path, mode, side, quantity, and entry offset.
- [x] Added a stable `StrategyExperimentResult` schema with requested, filled, and remaining quantity; fill rate; average execution price; decision metrics; implementation shortfall; and time-to-fill fields.
- [x] Added CSV output for strategy-experiment results, including tested passive/aggressive strategy labels and result rows.
- [x] Added unit-test coverage for strategy-experiment configuration/helpers, adapter initialization and decision-metric capture, runner scaffolding, and CSV writing.
- [x] Strategy comparison under replay runs passive and aggressive configurations against the same replay events and entry offset.
- [x] Injected-order fills are linked from matching-engine trades into experiment results, including filled quantity, remaining quantity, fill rate, and weighted average execution price.
- [x] Decision-time top-of-book state is captured immediately before injected-order submission, with `BeforeEvent` and `AfterEvent` timing semantics preserved.
- [x] Sign-aware implementation shortfall is calculated when a valid decision-time mid-price and execution price are available.
- [x] Deterministic replay pacing supports unpaced and event-time modes, a configurable speed multiplier, a wall-clock implementation, and a recording test clock.
- [ ] Timestamp-based time-to-fill remains deferred because the matching-engine timestamps are synthetic sequence values rather than normalized replay-time microseconds.
- [ ] Queue-position-aware experiments still depend on richer source data and more complete order-lifecycle reconstruction.

## Suggested implementation order right now

Priority order for the next few steps:

1. Stabilize the current replay files and adapter structure.
2. Plug the adapter into the real matching engine interface.
3. Add replay-focused tests.
4. Define snapshot output.
5. Start feature extraction from replayed book state.
6. Revisit Python bindings only after the replay/core path feels stable.

---

## Definition of success

Bookforge will feel “high level” when it can do the following:

- replay a historical sample deterministically,
- map raw market data into a clean internal engine interface,
- maintain a correct order-book state,
- export useful microstructure features,
- support a reproducible Python research workflow,
- and present the system clearly through tests, docs, and a small demo.

At that point, it becomes a strong portfolio project not just as an order book, but as a compact **market microstructure research platform**.