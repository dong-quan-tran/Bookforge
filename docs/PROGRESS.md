Progress — July 9, 2026
Set up the initial Bookforge project layout and docs blueprint for the C++ order book core and future microstructure features.

Implemented the first version of OrderBook and PriceLevel (adds, cancels, executes top order, best bid/ask, mid-price, spread, depth queries).

Fixed a compile issue in ExecuteTopOrder caused by using a ternary with two different std::map types, and rewrote it using clear if/else branches.

Ran the CMake debug build and confirmed all current Google Tests for the order book are passing.

Committed the core implementation and tests as a single focused commit to keep history clean and understandable.

Progress log: 07/10/2026

Core implementation and refactors
Implemented PriceLevel and OrderBook in src/core/price_level.* and src/core/order_book.* with:

Bid/ask side maps:

bids_: std::map<double, PriceLevel, std::greater<>>

asks_: std::map<double, PriceLevel, std::less<>>

Per-level storage of orders in FIFO order using std::list<Order>.

Introduced an OrderLocation struct and an order-id index:

std::unordered_map<std::uint64_t, OrderLocation> order_index_ for direct lookup by order id.

OrderLocation now holds { side, price, PriceLevel::Iterator order_it } so cancel can erase in O(1) at the list node.

Switched PriceLevel from std::deque to std::list:

PriceLevel::AddOrder now returns an iterator to the inserted Order.

PriceLevel::RemoveOrder erases by iterator, eliminating per-level linear scans for cancellation.

Public API and behavior
OrderBook public API after today:

bool AddOrder(const Order& order);

bool CancelOrder(std::uint64_t order_id);

bool ExecuteTopOrder(Side side, double price, std::uint32_t quantity);

Query methods: GetBestBid, GetBestAsk, GetMidPrice, GetSpread, GetLevelVolume, GetBidDepth, GetAskDepth.

Added a duplicate-id guard in AddOrder:

If order_index_ already contains order.id, AddOrder returns false and does not change the book state.

This keeps the order index and price levels consistent even under accidental repeated ids.

Testing
Expanded tests/cpp/test_order_book.cpp to 22 tests, covering:

Empty book behavior for best bid/ask, mid, spread.

Single and multiple orders, same-level aggregation, and best bid/ask tracking.

Depth queries (GetBidDepth, GetAskDepth) including requests for more levels than exist.

Partial and full execution at a price level, with automatic level removal when empty.

Cancel behavior:

CancelOrderRemovesLevelWhenEmpty

CancelMissingOrderReturnsFalse

CancelOrderByIdRemovesAsk

FullExecutionRemovesOrderFromIndex (ensures ids are removed on full fills).

FIFO semantics at a price level:

ExecuteTopOrderConsumesOldestOrderFirst

ExecuteTopOrderPartiallyConsumesOldestBeforeNextOrder

Edge queries:

MidPriceUnavailableWhenOnlyBidExists

SpreadUnavailableWhenOnlyAskExists

Duplicate-id behavior:

AddDuplicateOrderIdReturnsFalse

DuplicateOrderIdDoesNotChangeBookState (best bid, level count, and volume unchanged).

Benchmarking
Added a simple latency benchmark executable:

Target: benchmark_order_book in bench/benchmark_order_book.cpp.

Uses std::chrono::steady_clock to time repeated operations, which is recommended for interval timing.

Benchmarked operations with iterations = 100000 on Windows Debug build:

AddOrder: total_ns=111230700, avg_ns≈1112 ns/op.

CancelOrder: total_ns=4831089700, avg_ns≈48310 ns/op.

ExecuteTopOrder: total_ns=73795936200, avg_ns≈737959 ns/op.

Wired the benchmark into CMake:

add_executable(benchmark_order_book bench/benchmark_order_book.cpp).

Linked against bookforge_core.

Documentation
Updated docs/ARCHITECTURE.md to capture:

Current Week 1 scope and what’s implemented.

Data structures (std::map per side, std::list per level, std::unordered_map index).

Benchmark methodology (steady_clock, iterations, Debug build, operations measured).

Baseline latency numbers from today’s benchmark run.

Current status vs Week 1 blueprint
Define Order struct: Done.

Implement PriceLevel core: Done, now using std::list and iterators.

Implement OrderBook core: Done, including id index.

Add AddOrder, CancelOrder, ExecuteOrder:

Implemented as AddOrder, CancelOrder, ExecuteTopOrder.

Add GetBestBid, GetBestAsk, GetMidPrice, GetSpread: Done.

Write 20+ Google Test cases: 22 tests passing via CTest.

Add latency benchmark: Done (benchmark_order_book).

Record results in docs/ARCHITECTURE.md: Draft done with current baseline.

# Bookforge Progress Log — 2026-07-12

## Summary

Today’s work focused on expanding the matching engine, tightening order book semantics, and improving test reliability. The main outcomes were multi-level limit matching, maker-aware trade generation, replace/reduce order behavior, self-trade prevention with both `CancelNewest` and `CancelOldest`, and stronger test ergonomics through a shared order factory helper.

## Completed work

### Matching engine

- Added limit-order matching through a dedicated `MatchingEngine` entry point.
- Implemented multi-level matching so an incoming order can walk the book across multiple price levels.
- Added maker-aware trade generation so each trade records both taker and maker order IDs.

### Order book behavior

- Added order reduction semantics.
- Added order replacement semantics.
- Verified that reduction preserves priority while replacement causes the order to lose time priority.
- Added tests for same-price and new-price replacement behavior.

### Self-trade prevention

- Added `SelfTradePrevention::CancelNewest`.
- Updated tests to cover `CancelNewest` behavior for both buy-side and sell-side matching.
- Added `SelfTradePrevention::CancelOldest`.
- Added tests verifying that `CancelOldest` removes the resting self-matching order and allows the incoming order to continue matching or rest.

### Test infrastructure

- Added a `MakeOrder` helper to reduce repetitive aggregate initialization in tests.
- Updated tests to use the current `Order` shape consistently.
- Expanded matching engine coverage with maker-aware and self-trade-prevention scenarios.
- Kept the order book test suite aligned with reduce, replace, FIFO, and depth behavior.

## Commits completed today

- `50f048f` — Add matching engine for limit orders
- `00b79a9` — Add multi-level limit order matching
- `7875f91` — Add order reduction and replace semantics
- `31405d3` — Add maker-aware trade generation
- `2625e2b` — Add self-trade prevention CancelNewest
- `3a9ae00` — Update tests with CancelNewest
- `de4c756` — Add MakeOrder helper
- `36ce2e3` — Add CancelOldest
- `e7b4d23` — Add tests for CancelOldest

## Validation

- Rebuilt the project successfully with CMake.
- Confirmed the test suite passed in full.
- Current status: 51 tests passing, 0 failing.

## Notes

This was a strong correctness-focused session. The matching engine now supports more realistic execution behavior, self-trade prevention covers two useful policies, and the test suite is in a much better position to support future refactors.

# Bookforge Progress Log — 2026-07-13

## Summary

Today’s work focused on improving the order book benchmark file and getting it back into a clean, maintainable state before starting Week 2.

## Completed work

- Updated `bench/benchmark_order_book.cpp` to use the current `Order` shape.
- Added a local `MakeOrder` helper to reduce brittle aggregate initialization in the benchmark file.
- Kept the benchmark setup clearer by separating book population from the timed sections for cancel and execute paths.
- Rebuilt and ran the benchmark successfully.
- Captured baseline timing for core order book operations:
  - `AddOrder`
  - `CancelOrder`
  - `ExecuteTopOrder`

## Result

The benchmark now compiles cleanly and provides a usable baseline for future performance work. With that foundation in place, benchmark improvements are in a good stopping point, and Week 2 can begin with a stable reference point.

## 2026-07-16

### Completed
- Explored the Hyperliquid order-status dataset layout and verified the extracted folder structure.
- Loaded and decoded `statuses.csv`, then mapped raw `statusId` values to readable status names.
- Built a Python enrichment flow to merge raw order events with status labels and derive a simplified `eventType`.
- Generated an enriched sample event CSV for replay experiments.
- Added initial C++ replay-side types:
  - `ExternalOrderEvent`
  - `HyperliquidCsvReader`
  - `HyperliquidMessageReplayer`
- Added a basic replay executable to read the enriched CSV and summarize event counts.
- Added an adapter-oriented replay design:
  - `HyperliquidEngineAdapter`
  - `ReplayEngineStub`
- Updated the main `README.md` to reflect the new Hyperliquid replay workflow and current architecture direction.
- Cleaned up git tracking so large raw datasets stay local and only code/docs are committed.

### Key outputs
- Working enriched sample schema:

```text
ts,limitPx,sz,isAsk,statusId,status,eventType
```

- First external-event replay path from CSV into C++.
- First adapter boundary between exchange-specific replay data and internal engine-facing actions.

### Notes
- The current Hyperliquid sample is good enough for replay experiments and event classification.
- Exact cancel/fill semantics are still limited because the current sample workflow does not yet fully model order-ID-based lifecycle reconstruction.

### Next
- Replace the stub path with the real matching engine interface.
- Define how `New`, `Cancel`, and `Fill` should map into internal engine actions.
- Add replay-focused tests for parsing, event classification, and adapter behavior.

## 2026-07-17 — Replay foundation and testing

### Summary

Today’s work pushed Bookforge’s replay layer from an early prototype into a **deterministic, test-backed subsystem**.

Main outcomes:
- Closed out the Phase 1 testing checklist.
- Added a Hyperliquid replay adapter that drives the real matching engine.
- Introduced replay configuration and deterministic replay runner abstractions.
- Hardened the Hyperliquid CSV reader with malformed-row handling.
- Added parser tests, bounded replay tests, and replay regression coverage.
- Updated the Phase 2 checklist to reflect completed replay infrastructure work.

### Completed work

#### Core documentation and checklist
- Documented order book invariants and ownership rules.
- Marked the Phase 1 testing checklist complete.

#### Hyperliquid replay integration
- Added a Hyperliquid-specific adapter that maps external replay events into matching engine actions.
- Wired the Hyperliquid replay executable into the real matching engine adapter.
- Added integration tests for Hyperliquid replay adapter behavior.
- Registered Hyperliquid replay adapter tests in CMake.

#### Replay infrastructure
- Added `ReplayConfig` to hold replay parameters such as source, file path, symbol, offsets, limits, and logging settings.
- Added `ReplayRunner` to replay events in deterministic input order.
- Wired the replay entry point to use the new config and runner abstractions.

#### CSV reader hardening
- Extended `HyperliquidCsvReader` with strict and non-strict parsing modes.
- Added malformed-row handling with optional logging.
- Kept replay behavior deterministic even when skipping bad rows in non-strict mode.

#### Test coverage
- Added parser tests for valid rows, malformed rows, strict-mode failure, and unknown status mapping.
- Added bounded replay tests for `start_offset` and `max_events`.
- Added replay regression coverage and registered it in CMake.

### Commits

- `941aa4a` — Add Hyperliquid adapter for matching engine replay events
- `2a9cdc7` — Wire Hyperliquid replay main into real matching engine adapter
- `05e908b` — Add replay adapter integration tests for Hyperliquid events
- `e1c7a0b` — Register Hyperliquid replay adapter tests in CMake
- `0d28830` — Document order book invariants and ownership rules
- `0493c6d` — Mark Phase 1 testing checklist complete
- `8448044` — Add replay config and runner interfaces
- `8215d4a` — Add malformed-row handling to Hyperliquid CSV reader
- `07aee51` — Wire replay main to config and deterministic runner
- `d7487a6` — Add parser tests for Hyperliquid CSV reader
- `5a2c8ab` — Register replay runner and CSV reader tests in CMake
- `ba8ab8a` — Update Phase 2 replay checklist
- `e16280b` — Add bounded replay tests for deterministic runner
- `7fea817` — Register replay regression

### Phase status

#### Phase 1 — C++ core
Phase 1 testing is now marked complete.

#### Phase 2 — Replay foundation
Completed today:
- Stable external event model
- Replay interfaces in `src/replay/`
- CSV / flat-file replay reader
- Deterministic event ordering guarantees
- Replay statistics and instrumentation
- Replay logging for debugging
- Malformed-row error handling
- Replay configuration object

Also strengthened:
- Parser tests for malformed / missing data
- Bounded replay testing
- Replay regression coverage

### Remaining Phase 2 work

Still open:
- Support LOBSTER-style message replay
- Document source-specific field mappings
- Finalize fixture-based replay regression tests if more stable fixtures are needed

### Notes

The replay path is now strong enough to support:
- controlled sample replays,
- regression-friendly testing,
- future expansion to additional historical data sources.

# Progress Log — 2026-07-18

## Summary

Today’s work moved Bookforge forward across three major areas:

- replay adapter abstraction and test coverage,
- matching-engine replay integration and Hyperliquid regression stability,
- Phase 5 snapshot and serialization infrastructure, including round-trip CSV validation.

By the end of the day, the replay path was cleaner, regression coverage was more reliable, and the snapshot layer gained a full documented read/write workflow with automated tests.

## Completed work

### Replay adapter layer

#### Add generic replay adapter interface
- Introduced a generic replay adapter abstraction to decouple replay orchestration from the Hyperliquid-specific adapter.
- Established a cleaner interface boundary between replay inputs and matching-engine behavior.

#### Refactor Hyperliquid adapter onto replay interface
- Moved the Hyperliquid adapter onto the generic replay interface.
- Reduced source-specific coupling in the replay runner and made future source integration easier.

#### Add replay adapter tests and CMake target
- Added dedicated replay adapter tests.
- Wired the new replay adapter test target into CMake and CTest.

#### Update blueprint for Phase 3 adapter layer
- Updated planning documentation to reflect the adapter refactor and its testing coverage.

### Replay integration and regression hardening

#### integrate matching engine with replay and add integration tests
- Connected replay more directly to the matching engine.
- Added integration tests covering replay-style event sequences and end-to-end engine behavior.

#### Fix Hyperliquid replay regression tests and fixture paths
- Fixed fixture path handling for replay regression tests.
- Restored stable execution of Hyperliquid replay regression coverage under CTest.

#### Document Hyperliquid CSV to ExternalOrderEvent field mapping
- Documented how Hyperliquid CSV fields map into `ExternalOrderEvent`.
- Improved traceability from raw replay data to internal event representation.

### Phase 5 snapshot and serialization layer

#### add order book snapshot, CSV serializer, comparator
- Added the core snapshot model for reconstructed book state.
- Implemented CSV snapshot serialization.
- Implemented snapshot comparison utilities for deterministic checkpoint validation.

#### Add test for snapshot
- Added the initial snapshot test target and baseline test coverage.
- Verified top-of-book values, depth export, and comparator behavior.

#### Update Phase 5 progress on blueprint
- Updated the project blueprint to reflect the current implementation state of Phase 5.

#### Add snapshot schema documentation
- Added Markdown documentation describing the snapshot schema, CSV layout, and comparison semantics.

#### Phase 5: add snapshot CSV deserializer
- Implemented CSV deserialization for snapshots.
- Added header validation and reconstruction of scalar fields plus top-N depth.

#### Phase 5: add snapshot CSV round-trip tests
- Added round-trip tests covering write → read → compare behavior.
- Strengthened reproducibility guarantees for exported snapshot state.

## Technical outcomes

### Replay architecture
- Replay is now structured around a generic adapter interface rather than a single source-specific adapter.
- This makes the system easier to extend to additional sources such as LOBSTER.

### Testing stability
- Hyperliquid replay regression coverage is now stable and fixture-backed.
- Matching-engine integration tests provide stronger confidence in replay-driven state evolution.

### Snapshot system
- Book state is now exportable in a structured CSV format.
- Snapshots include:
  - symbol and replay metadata,
  - event counters,
  - best bid / ask,
  - spread and mid-price,
  - top-N bid and ask depth.
- Snapshots can now be serialized, deserialized, compared, and round-trip tested.

## Files and areas touched

### Core engine and replay
- replay adapter interface
- Hyperliquid replay adapter
- replay runner integration
- matching-engine integration tests
- replay regression tests and fixtures
- replay-related CMake targets

### Snapshot layer
- `BookSnapshot`
- `SnapshotBuilder`
- `SnapshotSerializer`
- `SnapshotComparator`
- `SnapshotDeserializer`
- snapshot unit tests
- snapshot schema documentation
- Phase 5 blueprint updates

## Result at end of day

The project ended the day with:

- a cleaner replay abstraction,
- stronger replay and integration test coverage,
- a functioning snapshot serialization layer with documentation,
- a working CSV round-trip path for reproducible book-state export.

## Next steps

### Phase 5 follow-up
- Add binary snapshot output if still desired.
- Expand replay checkpoint validation using larger or multiple fixtures.
- Continue refining snapshot tooling if more formats or comparison modes are needed.

### Forward-looking work
- Use the new replay and snapshot foundation to support future source ingestion and deeper replay analysis workflows.

# Progress Log — 2026-07-19

## Overview

Today’s work moved Bookforge from a replay-and-snapshot engine toward a research-ready pipeline. The main outcomes were:

- Finished the remaining snapshot serialization work in Phase 5.
- Completed Phase 6 feature extraction, including OFI, weighted OFI, rolling context features, and Python-side CSV validation.
- Started Phase 7 by adding the initial pybind11 binding module and updating the blueprint for the Python bridge and research layer.

The project now has a cleaner end-to-end path:

`replay -> snapshots -> features -> validated CSV -> Python -> ML`

## Commits

### Phase 7
- `72351eb` — Docs: update Phase 7 blueprint checklist
- `99eda64` — Phase 7: add initial pybind11 binding module

### Phase 6
- `36ce93e` — Phase 6: mark feature export validation complete
- `3019611` — Phase 6: add Python feature export validator
- `510ce2a` — Phase 6: add rolling context feature tests
- `19e6188` — Phase 6: add rolling liquidity and volatility context features
- `1a449d4` — Phase 6: add weighted OFI tests
- `3b9bfc0` — Phase 6: add weighted OFI feature
- `e249663` — Phase 6: add OFI feature tests
- `da57f8c` — Phase 6: add OFI feature builder
- `e278522` — Phase 6: add feature extraction tests and schema notes
- `2b15d77` — Phase 6: add base feature extraction from snapshots

### Phase 5
- `66eaaed` — Phase 5: add binary snapshot round-trip tests
- `e05d932` — Phase 5: add binary snapshot serialization support

## Phase 5 Progress

### Binary snapshot serialization
Added binary snapshot serialization support so book state can be exported and restored in a compact machine-friendly format.

### Binary round-trip tests
Added round-trip tests to verify that serialized binary snapshots can be deserialized back into equivalent in-memory state.

### Result
Phase 5 is effectively complete from the implementation and correctness side. Snapshot data now supports both richer persistence and stronger regression testing.

## Phase 6 Progress

### Base feature extraction
Implemented base feature extraction from snapshots, including:

- Best bid
- Best ask
- Spread
- Mid-price
- L1 bid quantity
- L1 ask quantity
- L1 depth imbalance
- Multi-level depth aggregates
- Multi-level depth imbalance

Also added schema notes and supporting tests so the feature table has a stable, explicit structure.

### OFI feature builder
Implemented Order Flow Imbalance features:

- `ofi_l1`
- `ofi_lN`

This established the first event-sensitive microstructure signal on top of static snapshot-derived features.

### OFI tests
Added fixture-style tests for OFI behavior, covering best-level changes, depth changes, and multi-level aggregation logic.

### Weighted OFI
Extended OFI with a weighted multi-level variant:

- `weighted_ofi_lN`

The weighting scheme prioritizes near-touch levels over deeper ones, making the feature more aligned with practical microstructure intuition.

### Weighted OFI tests
Added tests that verify:

- Deeper levels are discounted correctly.
- Weighted OFI matches unweighted OFI when only L1 changes.

### Rolling liquidity and volatility context features
Added rolling-window context features, including:

- `rolling_mean_spread`
- `rolling_mean_l1_total_depth`
- `rolling_mean_lN_total_depth`
- `rolling_mid_return`
- `rolling_realized_mid_vol`
- `rolling_mean_abs_ofi_l1`
- `rolling_mean_abs_ofi_lN`

These features give each row local market context rather than relying only on instantaneous state.

### Rolling context tests
Added tests for rolling means, rolling returns, realized volatility, and rolling absolute OFI values.

### Python feature export validator
Added a Python validation script for exported feature CSVs. The validator checks:

- Required columns exist.
- Numeric columns are readable as numeric.
- Replay indices and timestamps are monotone.
- `spread == best_ask - best_bid`
- `mid_price == (best_bid + best_ask) / 2`
- Imbalance values remain within expected bounds.
- Non-negative fields stay non-negative where appropriate.

### Phase 6 completion
Marked feature export validation complete in the blueprint, which closes Phase 6.

### Result
Phase 6 is now in good shape:

- Features are generated in C++.
- Core feature logic is tested.
- Export schema is documented.
- Python can validate exported feature files before research or training begins.

## Phase 7 Progress

### Initial pybind11 binding module
Started the Python bridge by:

- Adding pybind11 through CMake.
- Creating the initial binding module under `src/python/`.
- Wiring a `bookforge_py` extension module against `bookforge_core`.

### First bound objects
The initial binding layer targets plain data structs first:

- `DepthLevelSnapshot`
- `BookSnapshot`
- `FeatureRow`

This is the right first bridge because it exposes stable value objects to Python before attempting more complex runtime control paths like replay orchestration.

### Blueprint update
Updated `docs/BLUEPRINT.md` to reflect the new Phase 7 direction and the near-term order of work for the research layer.

### Result
Phase 7 has started successfully. The Python bridge is no longer just a plan; it now has an initial implementation path in the build system and source tree.

## Current Project State

### Completed or effectively completed
- Phase 5 — Snapshot and serialization layer
- Phase 6 — Feature extraction

### Started
- Phase 7 — Python bridge and research layer

## Key accomplishments today

- Added binary snapshot serialization and round-trip coverage.
- Built the full first-pass microstructure feature stack.
- Added OFI and weighted OFI.
- Added rolling liquidity and volatility regime features.
- Added Python-side feature export validation.
- Closed out Phase 6 in the blueprint.
- Began the pybind11 bridge for Python research workflows.

## Next steps

The next logical tasks for Phase 7 are:

1. Build the Python wrapper package.
2. Implement Python-accessible feature and snapshot loading utilities.
3. Implement short-horizon label generation.
4. Build the first training dataset.
5. Train an XGBoost baseline.
6. Add walk-forward validation and feature importance analysis.

## Notes

Today was a strong transition point for the project. Bookforge now has a clearer path from C++ market replay infrastructure into Python-based research and modeling, with most of the needed data plumbing already in place.

# Bookforge Progress Log — 2026-07-20

## Summary

Today's work closed out the remaining Phase 7 checklist items around the Python research layer and tightened the repo's packaging, validation, and project documentation. The main outcomes were a finalized Python wrapper package layout, successful end-to-end ML baseline runs with walk-forward validation, SHAP and MLflow integration verification, and pytest coverage for the Python wrappers.

## Packaging and wrapper cleanup

The Python/C++ wrapper layout was finalized around a split structure: the C++ binding source remains in `src/python/bookforge_py.cpp`, while the importable Python package lives under the root `python/` directory. This matches a common pybind11 packaging pattern where the extension module and the surrounding Python package are kept logically separate, making it easier to expose a clean top-level API through `__init__.py` while preserving a conventional native-source layout.

The package export surface was then fixed so the Python training code could access dataset utilities directly through `import bookforge_py as bf`. In practice, that meant updating `python/bookforge_py/__init__.py` to re-export dataset classes and helpers such as `walk_forward_splits`, which resolved the `AttributeError` encountered during the first walk-forward training attempt.

## ML baseline validation

The ML stack was validated in both holdout and walk-forward modes using `python/ml/train.py`. A holdout classification run completed successfully and confirmed that the feature-loading, dataset-construction, and training path was functioning end to end with MLflow enabled.

Walk-forward validation initially failed because `walk_forward_splits` was not exported from the Python package. After fixing the package exports, the walk-forward pipeline completed over five folds with aggregate accuracy of 0.999 and a minimum fold accuracy of 0.995, confirming that the validation infrastructure, fold generation, and metric aggregation path are operational.

The training output also revealed a substantive modeling issue: the label distribution is highly imbalanced, and one fold contained only 50 examples of class `1` versus 9,950 examples of class `0`. That means the near-perfect accuracy is mostly a reflection of class imbalance rather than strong predictive performance, so the next research task should focus on label design and class-balance diagnostics rather than additional training infrastructure.

## SHAP and MLflow

The SHAP-enabled training path was exercised successfully after correcting a PowerShell command typo that had appended a trailing backslash to the `--shap-sample-size` argument. Once that argument was passed correctly, the command completed with `--enable-shap`, which verified that the explainability branch of the training pipeline is wired correctly.

MLflow tracking was also validated as part of the same workflow using a local SQLite backend via `MLFLOW_TRACKING_URI=sqlite:///mlruns.db`. This confirmed that the project now supports repeatable experiment tracking with logged metrics and artifacts for baseline runs, which materially improves reproducibility for future model comparisons.

A repo hygiene decision was made not to commit `mlruns.db`, since it is an environment-specific experiment artifact rather than source code. The correct action is to ignore `mlruns.db` alongside any MLflow run directories in `.gitignore` to keep version control focused on code and durable documentation rather than local tracking state.

## Test coverage for Python wrappers

The last unchecked Phase 7 item, pytest coverage for the Python wrappers, was addressed by adding focused tests under `tests/python/` for the dataset and loader utilities. These tests covered dataset construction, chronological splitting, walk-forward fold generation, feature-frame validation, and feature/metadata column separation.

A couple of tests had to be adjusted to match the actual behavior of the current code rather than an idealized future state. Specifically, one dataset-label test was relaxed because the current synthetic test setup can produce a single classification label after normalization, and one loader monotonicity test was updated after it became clear that the current `validate_feature_frame` behavior does not raise on the specific duplicate-index case originally assumed by the test.

After those refinements, the wrapper test suite reached a passing state, which is enough to close the checklist item for Python wrapper coverage in the current phase. This does not mean the research layer is “finished,” but it does mean the wrapper boundary now has baseline regression protection and the Phase 7 testing goal can reasonably be marked complete.

## Dependency review

The project's `requirements.txt` was reviewed against the actual code paths exercised today. No additions were required because the file already includes the key dependencies needed for the current feature set: pandas, numpy, scipy, scikit-learn, XGBoost, SHAP, MLflow, pybind11, FastAPI-related packages, and pytest tooling.

The main recommendation was organizational rather than functional: later on, it may be worth splitting runtime and development dependencies so test-only packages such as `pytest` and `pytest-cov` can live in a development requirements file. That is an optional cleanup task rather than a blocker for the current phase.

## README refresh

The general README was found to be out of date because it still described several feature-export and ML capabilities as planned work. A full replacement README was drafted to reflect the current state of the repo, including the updated package layout, feature export pipeline, Python ML workflow, walk-forward validation, SHAP analysis, MLflow tracking, and current roadmap status.

The updated README also adds concrete command-line examples for feature export, holdout training, walk-forward evaluation, SHAP usage, and MLflow UI launch. That makes the project materially easier to run for a new reader and better aligns the README with the reproducible workflow that was actually tested today.

## Suggested commits from today’s work

The work naturally grouped into a few focused commits:

- Wrapper/export fix: `python/bookforge_py/__init__.py`, `python/bookforge_py/dataset.py`
- ML training package and trainer updates: `python/ml/__init__.py`, `python/ml/train.py`
- Python wrapper tests: `tests/python/test_dataset.py`, `tests/python/test_loaders.py`
- Documentation refresh: `README.md`, and optionally `docs/BLUEPRINT.md` if Phase 7 items were checked off there
- Repo hygiene: `.gitignore` update for `mlruns.db`

This commit breakdown keeps infrastructure, ML workflow, testing, and documentation changes logically separated, which should make the project history easier to review later.

## Status after today

Phase 7 is now effectively wrapped up from an implementation perspective: the wrapper package is in place, the training pipeline works in holdout and walk-forward modes, SHAP and MLflow are integrated, and Python wrapper tests are passing. The most important next step is no longer plumbing; it is improving label construction and evaluation diagnostics so the baseline model results become analytically meaningful rather than merely operational.


# Progress Log — 2026-07-21

## Summary

Today focused on completing the first usable API + dashboard demo layer for Bookforge and cleaning up the repo structure around it. The main outcome was getting the FastAPI backend and React/Vite dashboard running locally end to end, with replay summary cards and feature charts rendering successfully.

---

## Completed today

### Dashboard debugging and bring-up
- Fixed the frontend import issue caused by the CSS filename mismatch in `main.jsx`.
- Verified the Vite frontend builds successfully with `npm run build`.
- Confirmed the dashboard runs locally with `npm run dev`.
- Debugged the blank-page runtime issue after build success.
- Updated `App.jsx` to better match the actual API response schema.
- Fixed chart data mapping to use the real feature fields returned by the backend.
- Filtered sparse / null-heavy early sample rows so charts render meaningful data.
- Confirmed the dashboard now loads and renders:
  - replay summary cards
  - mid-price chart
  - spread chart
  - L1 bid vs ask depth
  - L1 depth imbalance

### API and backend work
- Continued the FastAPI service cleanup and separation.
- Moved API logic toward a clearer structure using:
  - `main.py`
  - `schemas.py`
  - `services.py`
- Removed the older `routes.py` layout in favor of the updated organization.
- Verified the replay summary and feature sampling endpoints are working with the dashboard.

### Repo structure cleanup
- Removed the old top-level `Dockerfile`.
- Added `Dockerfile.api`.
- Updated `docker-compose.yml`.
- Removed the old `python/dashboard/App.jsx` location in favor of the standalone `dashboard/` frontend app.
- Added the new `dashboard/` app to the repo structure.
- Added `tests/python/test_api.py`.
- Updated `requirements.txt` to support the current API/demo stack.

### Documentation updates
- Updated the Phase 8 checklist to reflect current progress.
- Drafted an updated top-level README that reflects:
  - the current FastAPI backend
  - the Vite + React dashboard
  - Docker / local demo workflow
  - the revised repo layout

---

## Current result

The local demo stack now works in a meaningful way:

- FastAPI backend serves replay summary and sampled feature data.
- React/Vite dashboard loads successfully in the browser.
- Dashboard visualizes exported replay features from `output/features.csv`.
- The project now has a presentable inspection/demo layer on top of the replay and feature pipeline.

---

## Files touched

### Modified
- `docker-compose.yml`
- `docs/BLUEPRINT.md`
- `python/api/__init__.py`
- `python/api/main.py`
- `requirements.txt`

### Added
- `Dockerfile.api`
- `dashboard/`
- `python/api/schemas.py`
- `python/api/services.py`
- `tests/python/test_api.py`

### Removed
- `Dockerfile`
- `python/api/routes.py`
- `python/dashboard/App.jsx`
- `python/lob_engine/__init__.py`
- `python/lob_engine/lob_wrapper.py`

---

## Phase impact

### Phase 8 — API and dashboard
The following items are now effectively complete:
- Build FastAPI service
- Add endpoint for replay summary
- Add endpoint for feature export / sample retrieval
- Build React dashboard
- Visualize spread, mid-price, depth, and imbalance
- Add replay summary cards and charts
- Add API tests
- Add Docker setup
- End-to-end local demo
- Basic API contract tests
- Dashboard loads sample replay outputs correctly

Still open:
- Add endpoint for book snapshot inspection

---

## Notes and observations

- The main frontend issue was no longer the port configuration; it was a mismatch between the dashboard’s expected schema and the actual JSON returned by the API.
- Early sampled replay rows contain many null values, so frontend filtering/mapping was necessary to make the charts useful.
- Separating the dashboard into its own `dashboard/` app makes the project layout cleaner and more realistic for demo/deployment workflows.
- The API/demo layer is now strong enough to show as part of the portfolio narrative, even though snapshot inspection and some polish items are still pending.

---

# Progress Log — 2026-07-26

## Summary

Today focused on moving the project into Phase 9 polish work: tightening the repo structure, adding CI, adding lint/format configuration, and cleaning up Python code around the new API and research stack. The main theme was shifting the repo from “functional” toward “maintainable and interview-ready.”

---

## Completed today

### CI and repo hygiene
- Added GitHub Actions CI for the project.
- Configured CI to cover the C++ build, C++ tests, and Python tests.
- Added Python lint/format configuration through `pyproject.toml`.
- Added `.clang-format` for future C++ formatting enforcement.
- Updated the CI pipeline to support linting and formatting checks.
- Confirmed the repo structure is now better aligned with long-term maintenance.

### Python cleanup
- Ran Ruff over the Python codebase.
- Fixed unused imports and formatting issues reported by Ruff.
- Cleaned up several files in:
  - `python/bookforge_py/`
  - `python/ml/`
  - `tests/python/`
  - `tools/validate_feature_export.py`
- Normalized Python formatting in the updated files.

### API and dashboard polish
- Continued the new API inspection layer work.
- Verified the snapshot inspection endpoint and related API tests are committed.
- Kept the dashboard/API structure aligned with the current local demo workflow.

### Docs and planning
- Reviewed the latest README content against the current implementation.
- Confirmed the README needed only small status-sync updates rather than a full rewrite.
- Moved Phase 9 forward by identifying the next engineering polish tasks.

---

## Current result

The repo is now in a better place technically:

- CI is in place.
- Python formatting/lint rules are now defined.
- The API layer and dashboard are integrated into the current project story.
- The codebase is cleaner and closer to being a serious portfolio project.

---

## Files touched today

### Added
- `.github/workflows/ci.yml`
- `pyproject.toml`
- `.clang-format`

### Modified
- `docs/BLUEPRINT.md`
- `python/api/__init__.py`
- `python/api/main.py`
- `python/api/schemas.py`
- `python/api/services.py`
- `python/bookforge_py/__init__.py`
- `python/bookforge_py/dataset.py`
- `python/bookforge_py/impact.py`
- `python/bookforge_py/labels.py`
- `python/bookforge_py/loaders.py`
- `python/bookforge_py/test_import.py`
- `python/ml/__init__.py`
- `python/ml/train.py`
- `tests/python/test_api.py`
- `tests/python/test_dataset.py`
- `tests/python/test_loaders.py`
- `tools/validate_feature_export.py`

### Removed
- `setup.py`

---

## Commits made today

- Added GitHub Actions CI.
- Added lint and formatting configuration.
- Fixed Python dependency list.
- Committed the snapshot inspection endpoint and its tests earlier in the session.
- Committed the API and docs status updates separately.

---

## Phase impact

### Phase 9 — Performance and engineering polish
Completed or advanced:
- GitHub Actions CI
- formatting / linting rules
- improved repo hygiene
- README status alignment
- Python cleanup from Ruff output

Still open:
- Reach 50+ Google Tests
- Reach 50+ Pytest cases
- Add C++ benchmarks
- Add replay throughput benchmark
- Add profiling notes
- Finish `docs/INTERVIEW_PREP.md`
- Add architecture diagrams
- Tag a clean milestone release

---

## Notes

- C++ formatting has been prepared but not yet applied locally.
- The Windows environment needs a usable C++ formatter installation before that step can happen cleanly.
- The main priority for today was to land the CI and lint infrastructure first, which is now done.

---

## Next step

The next best Phase 9 task is likely documentation polish, especially:
- improving the README wording and status tables,
- finishing `docs/INTERVIEW_PREP.md`,
- and then returning to benchmarks and test-count growth.

# Bookforge Progress Log — 2026-07-27

## Overview
Today's work focused on CI stabilization, benchmark setup, replay benchmark integration, and repository cleanup after a divergent-branch merge. The branch history for the day includes a README update, CI workflow refactors, formatting consistency fixes, a merge from `origin/main`, benchmark-related code changes, and a final formatting-only commit for CI compliance.

## Commits completed
- [Update general readme](https://github.com/dong-quan-tran/Bookforge/commit/28e228ad0941456b974d6b21a653740ed2415d22)
- [Refactor CI workflow for linting and formatting steps](https://github.com/dong-quan-tran/Bookforge/commit/4e9c477ddb070aef527d1da5682b7f1fa0eacba5)
- [Refactor lint job in CI workflow](https://github.com/dong-quan-tran/Bookforge/commit/88723a9cbed2ed1eab1aacb1fa87d30620aab957)
- [add gitattributes to tackle local vs CI clang-format mismatch](https://github.com/dong-quan-tran/Bookforge/commit/40a5d185278f97133a3298a27ab677f4b8dedadc)
- [format C++ sources with clang-format](https://github.com/dong-quan-tran/Bookforge/commit/e88bd95b8aa4fd6a7898258a1ff7dbb9d8dbe69f)
- [fix Ruff import ordering in tests](https://github.com/dong-quan-tran/Bookforge/commit/f7c9f94a64117b03fe54fc91c6c6ded1cf115277)
- [fix Python test imports for repo package layout](https://github.com/dong-quan-tran/Bookforge/commit/3c1c5e4178a84367057ce4ed43af4dfdd7dcf282)
- [update C++ CSV header expectation](https://github.com/dong-quan-tran/Bookforge/commit/dd7f5188b92e97c1ff9c6a4f25ccdad88db180bf)
- [Merge origin/main into main](https://github.com/dong-quan-tran/Bookforge/commit/30cfd3f6bff7c08c0af73686bb6427c683cf4fdc)
- [Adjust CMake and order book benchmark](https://github.com/dong-quan-tran/Bookforge/commit/3de317c0921bc0cb6e9df4f90adea7d7114d5989)
- [Add replay benchmark](https://github.com/dong-quan-tran/Bookforge/commit/d1c03ea35caad513e7ffdf891efc1f67331f4b4a)
- [Format benchmark sources](https://github.com/dong-quan-tran/Bookforge/commit/fcf639aabfd8ddc67c6257f794a0ed015daa3cfc)

## Benchmark work
### Order book benchmark
The order book benchmark built and ran successfully in Release mode. The benchmark suite covered add, cancel, execute partial, execute full, reduce quantity, replace same price, and replace new price paths.

Example run command:

```powershell
.\build\bench\Release\benchmark_order_book.exe
```

Release build command used before running benchmarks:

```powershell
cmake --build build --config Release --parallel
```

### Replay benchmark
A new replay benchmark target and source file were added. The benchmark was updated several times to match the `HyperliquidCsvReader` interface, align with `ReplayRunner`, and handle the fact that the current fixture is very small.

Final replay benchmark command:

```powershell
.\build\bench\Release\benchmark_replay.exe
```

Current replay benchmark status:
- The executable builds and runs successfully.
- The current benchmark output is valid as a smoke test.
- The current fixture is too small to produce a representative replay throughput measurement.
- A larger historical CSV fixture is still needed for meaningful scaling and Phase 9 performance tracking.

## CI and formatting work
CI failed on `clang-format` violations in `bench/benchmark_replay.cpp` and `bench/benchmark_order_book.cpp`. The fix was to run `clang-format` directly on Windows using the full executable path, then commit the formatting-only change.

Windows PowerShell command used to format benchmark files:

```powershell
& "C:\Program Files\LLVM\bin\clang-format.exe" -style=file -i bench/benchmark_replay.cpp bench/benchmark_order_book.cpp
```

Alternative Visual Studio LLVM path if LLVM is not installed separately:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" -style=file -i bench/benchmark_replay.cpp bench/benchmark_order_book.cpp
```

Follow-up commit and push commands:

```powershell
git add bench/benchmark_replay.cpp bench/benchmark_order_book.cpp
git commit -m "Format benchmark sources"
git push origin main
```

## Git workflow cleanup
A merge was in progress after local and remote branches diverged. The merge was concluded first, then local work was split into separate commits for cleaner history.

Commands used for the cleanup flow:

```powershell
git commit -m "Merge origin/main into main"
git add CMakeLists.txt bench/benchmark_order_book.cpp
git commit -m "Adjust CMake and order book benchmark"
git add bench/CMakeLists.txt bench/benchmark_replay.cpp
git commit -m "Add replay benchmark"
```

History verification command:

```powershell
git log --oneline --graph --decorate -10
```

## Key outcomes
- CI workflow and formatting-related issues were addressed.
- Benchmark infrastructure now includes both order book and replay benchmark executables.
- Replay benchmark integration is complete at the code level.
- Replay benchmark measurement quality is currently limited by fixture size, not by the benchmark harness itself.
- Repository history was cleaned up into a sensible merge-plus-follow-up-commits sequence.

## Next steps
- Replace the replay fixture with a much larger CSV sample.
- Reintroduce multi-size replay benchmark arguments once the fixture supports them.
- Re-run benchmarks and capture a more representative replay throughput baseline.
- Verify the GitHub Actions run is fully green after the formatting commit.

# Progress Log — July 28, 2026

## Summary

Today focused on Phase 9 polish for Bookforge: documentation cleanup, test expansion, and milestone preparation. The repo is now in a much stronger state, with broader Python and C++ coverage, improved architecture docs, and a clean milestone tag in place.

## Completed work

### Documentation
- Rewrote and polished `docs/ARCHITECTURE.md`.
- Added architecture diagrams for the replay pipeline, core engine structure, and replay control flow.
- Updated `docs/INTERVIEW_PREP.md` with clearer project framing and stronger interview talking points.
- Refined `README.md` to better reflect current project scope, benchmarks, and usage.
- Updated `docs/BLUEPRINT.md` to reflect current Phase 9 progress.

### Replay benchmark update
- Expanded the replay benchmark fixture from a tiny smoke dataset to a large synthetic CSV fixture.
- `benchmark_replay.exe` now reports meaningful throughput instead of mostly benchmark overhead.
- Latest observed result on the large fixture was approximately 3.05M events/sec in Release mode on Windows.
- Interpretation: replay benchmark plumbing was correct; the prior flat result was caused by an undersized fixture.

### Testing
- Added C++ edge-case tests for:
  - order book behavior,
  - matching engine behavior,
  - replay runner bounds and ordering,
  - CSV reader parsing.
- Added Python edge-case tests for:
  - API pagination and snapshot inspection,
  - loader validation and column splitting,
  - dataset construction and walk-forward splitting.
- Confirmed all tests are green:
  - Python: 41 passed.
  - C++: 130 passed.

### Benchmarking
- Expanded the replay benchmark fixture so throughput measurements are meaningful.
- Kept the order book benchmark and replay benchmark both compiling and running successfully.

### Release / milestone
- Created and confirmed milestone tag `v0.1.0`.
- Tag points to the Phase 9 milestone commit and is an annotated release-style tag.

## Commit log

Commits from July 28, 2026:

- `4f970a2` — Polish architecture documentation
- `08513c7` — Polish project README
- `c519ecc` — Add API pagination and snapshot edge case tests
- `7467b67` — Add dataset and loader boundary tests
- `c9bdcb6` — Add replay runner and CSV reader edge case tests
- `a016905` — Add order book and matching engine edge case tests
- `6d2a8a8` — Update Phase 9 progress
- `63da3f4` — Add architecture diagrams
- `26facae` — Update interview prep material
- `93f4c0f` — Update README benchmark and project overview
- `26c0ffd` — Add large replay benchmark fixture
- `9f13bfc` — Update project architecture

## Current state

- The repo is in a clean milestone state.
- Core docs are substantially improved.
- Test coverage increased on both C++ and Python sides.
- Benchmarking is now more meaningful, especially for replay throughput.
- The Phase 9 milestone tag is in place and ready as a checkpoint.

## Next steps

- Continue toward the remaining Phase 9 checklist items if desired:
  - 50+ Google Tests.
  - 50+ Pytest cases.
  - final release polish if needed.
- Otherwise, treat `v0.1.0` as the Phase 9 milestone and move on to the next phase.

# July 29, 2026 Progress Log

Today’s work focused on pushing Bookforge’s replay tooling, documentation quality, and interview readiness forward in a coordinated way. The day started with simulation and replay improvements, moved through documentation and presentation work, and ended with cleanup, formatting, and blueprint updates.

## Replay and simulation work

The main systems change was extending replay support so historical runs can include scheduled injected orders. That work included replay adapter refactoring, test and CMake wiring cleanup, and added coverage for injected-order dispatch so the new path is exercised instead of remaining a loosely documented idea.

A synthetic market event generator was also added to produce replay-compatible CSV fixtures with deterministic seeding. That generator makes it easier to create stable benchmark inputs, demo scenarios, and feature-export datasets without depending entirely on external historical data.

## Documentation updates

Several commits improved the project’s documentation layer so the repository explains itself more clearly to recruiters, interviewers, and collaborators. Architecture decision records were added, benchmark documentation was written, and the README was updated to describe the replay fixture generator and how it fits into the workflow.

The architecture and interview-prep documents were then expanded in more depth. `INTERVIEW_PREP.md` now gives clearer answers on system design choices, market microstructure concepts, benchmark framing, trade-offs, and a tighter one-minute project pitch, while `ARCHITECTURE.md` now captures diagrams, invariants, replay flow, ownership rules, benchmarking intent, and Python binding boundaries.

## Demo and presentation work

The repo also gained a polished interview demo scenario, which strengthens the project’s presentation value beyond raw implementation. That matters because Bookforge is no longer just a matching-engine codebase; it now has a clearer story for showing deterministic replay, benchmarkability, and research-oriented outputs in a way that is easier to present live.

The benchmark and architecture additions also improved how the project can be discussed in interviews. Instead of describing the repo only as “a C++ order book,” the material now supports a fuller explanation of correctness-first design, replayability, extensibility, and where future optimizations would fit.

## Quality and maintenance

A meaningful part of today’s work was repo hygiene. Python tests were formatted and Ruff violations were fixed, replay and test C++ sources were formatted with `clang-format`, and `.gitignore` was updated so build and test artifacts stay out of version control.

Those cleanup commits are important because they reduce noise in future diffs and make the codebase feel more deliberate. Combined with the replay refactor, they also help keep CI and local development aligned instead of letting supporting infrastructure drift.

## Blueprint status changes

The Phase 10 stretch-goal checklist was updated to reflect what is now actually complete. In particular, user-injected orders during historical replay were marked done, and the current-status notes now explicitly mention replay-side scheduling abstractions, injected-order coverage, and the replay adapter’s expanded interface.

That update also clarified what remains unfinished. Strategy comparison under replay and queue-position-aware experiments are still open, which gives the next round of work a cleaner handoff.

## Commits completed today

- Add synthetic market event generator
- Update Phase 10 progress
- Add replay fixture generator
- Update readme with replay fixture generator
- Add architecture decision records
- Add interview demo scenario
- Add benchmark documentation
- Refactor replay adapter and CMake test wiring
- Add injected-order replay coverage
- Update gitignore for build and test artifacts
- Format Python tests and fix Ruff violations
- Format replay and test sources with clang-format
- Expand interview prep and architecture docs
- Update Phase 10 stretch goals checklist


July 30, 2026 — Progress Log
Today’s work focused on making the repository easier to work in and advancing the Phase 10 research scaffold.

Workflow and repo hygiene
Diagnosed repeated CI lint failures caused by formatting and line-ending drift.

Standardized the local formatting workflow around the Windows LLVM clang-format.exe.

Added .gitattributes guidance so Git normalizes line endings consistently.

Documented the local dev-check flow in the project docs so formatting, build, and test can be run together.

Kept formatting changes separate from functional changes to make future reviews cleaner.

Strategy experiment scaffold
Continued Phase 10 work on the passive-vs-aggressive replay experiment.

Added and refined the strategy experiment data structures.

Added CSV writing support for experiment results.

Added tests for experiment helpers and CSV output.

Kept the experiment scaffold green after fixing Windows file-handle cleanup in the tests.

Replay and build wiring
Added a minimal strategy experiment runner scaffold.

Wired the new files into CMake and kept the test suite passing.

Verified the full C++ test suite still passes after the new experiment-related changes.

Outcome
The repo now has a cleaner formatting workflow, less noisy Windows line-ending behavior, and a stronger foundation for the next Phase 10 task: turning the strategy experiment runner into a real replay-backed metric generator.

# August 5, 2026 — Progress Log

Today’s work focused on tightening the development workflow around formatting and tests, and wiring the strategy experiment harness end-to-end for Phase 10.

## Git hooks and formatting workflow

- Added a **pre-commit PowerShell script** (`scripts/pre-commit.ps1`) to automatically run `clang-format` over all C++ sources (`src`, `tests`, `bench`) and re-stage changes before each commit.
- Added a **pre-commit hook** (`.git/hooks/pre-commit`) that wraps the PowerShell script so formatting happens early in the Git workflow.
- Kept the **pre-push hook** (`.git/hooks/pre-push`) in place to run the local dev-check script before pushes: formatting, CMake configure, build, and CTest run as a safety net before code reaches CI.
- Updated `scripts/dev-check.ps1` to format all tracked C++ files with `clang-format -style=file`, then build Release/Debug and run the full C++ test suite.
- Verified that the hooks work on Windows by calling `powershell.exe` from the hook wrapper instead of `pwsh`, avoiding interpreter path issues.

This setup makes “edit → auto-format → commit → build/test → push” the default flow and reduces repeated lint failures on GitHub.

## Strategy experiment adapter and runner

- Introduced `StrategyExperimentAdapter`, a replay adapter derived from `IReplayAdapter`, which owns a `StrategyExperimentConfig`, `AdapterMetrics`, and a `StrategyExperimentResult` scaffold.
- Implemented the adapter constructor and `Result()` so that the result is initialized directly from the experiment config (mode, entry offset, side, limit price, requested quantity).
- Added `test_strategy_experiment_adapter.cpp` with GoogleTest coverage to verify that an adapter created from a config exposes the expected initial result state.
- Wired the adapter into CMake test targets so the adapter and result definitions are linked correctly for unit tests.

## Strategy experiment runner wiring

- Updated `StrategyExperimentRunner` so `RunOnce`:
  - constructs an `InjectedOrder` from the `StrategyExperimentConfig`,
  - builds an `InjectedOrderSchedule`,
  - constructs a `StrategyExperimentAdapter`,
  - calls `ReplayRunner::Run(adapter, events, schedule)`,
  - returns `adapter.Result()` as the experiment outcome.
- Updated `test_strategy_experiment_runner.cpp` to assert that `RunOnce` returns a result derived from the config, keeping tests focused on harness wiring rather than full execution behavior.
- Fixed CMake wiring so `test_strategy_experiment_runner` links all required sources: `StrategyExperimentRunner.cpp`, `StrategyExperiment.cpp`, `StrategyExperimentAdapter.cpp`, and `ReplayRunner.cpp`.

## Executable and CMake wiring

- Ensured both replay executables link the new experiment adapter and runner:
  - `hyperliquid_replay_main` now includes `StrategyExperimentAdapter.cpp`, `StrategyExperimentRunner.cpp`, `StrategyExperiment.cpp`, `StrategyExperimentCsvWriter.cpp`, `ReplayRunner.cpp`, and `HyperliquidCsvReader.cpp`.
  - `strategy_experiment_main` links the same stack, providing a dedicated entrypoint for single-trial strategy experiments.
- Cleaned up `CMakeLists.txt`:
  - organized test targets under `bookforge_add_test`,
  - added adapter and runner sources to the appropriate test and executable targets,
  - eliminated duplicate target declarations and missing-source link errors.

## Strategy experiment CLI skeleton

- Replaced `strategy_experiment_main.cpp` with a **CLI skeleton** that:
  - parses command-line arguments:
    - `--input` (required) for the replay CSV path,
    - `--output` for the result CSV path (default `output/strategy_experiment_results.csv`),
    - `--mode` (`passive` or `aggressive`),
    - `--side` (`buy` or `sell`),
    - `--quantity`,
    - `--entry-offset`,
  - prints the parsed options to stdout as a placeholder.
- Left a clear hook in the CLI for the next commit to:
  - load events via `HyperliquidCsvReader`,
  - build a `StrategyExperimentConfig` from the CLI options,
  - construct a `ReplayConfig` and `StrategyExperimentRunner`,
  - run a single experiment and write a CSV row via `StrategyExperimentCsvWriter`.

This gives a basic command-line interface around the experiment harness, ready to be fleshed out with real replay behavior and metrics.

## Phase 10 checklist updates

- Updated `docs/BLUEPRINT.md` to reflect that:
  - the synthetic event generator, injected order support, replay adapter, and experiment harness are now wired end-to-end,
  - the strategy experiment adapter, runner, and CLI provide a single-trial replay experiment entrypoint.
- Left “Compare passive vs aggressive strategy behavior under replay” and queue-position-aware experiments as open items, to be tackled after the CLI starts emitting real metrics and experiments are run over multiple trials.

## Outcome

By the end of the day:

- The repository has a clearer, automated formatting and test workflow via pre-commit and pre-push hooks.
- The strategy experiment harness (adapter + runner + CLI) is structurally complete and integrated into both executables and tests.
- Phase 10’s current status now explicitly recognizes the experiment plumbing as in place, setting up the next round of work: actually running passive vs aggressive experiments under replay and analyzing their behavior.