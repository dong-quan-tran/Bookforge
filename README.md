# Bookforge

Bookforge is a hybrid **C++ + Python** market-microstructure project for studying how a modern limit-order book behaves under replayed market-event flow.

At its core is a low-latency **C++20 matching engine** with a price-time-priority order book, deterministic replay infrastructure, multi-symbol replay routing, snapshot export, feature export, strategy-experiment scaffolding, and regression-tested historical event playback. On top of that, the repository includes a Python research layer for dataset construction, short-horizon machine learning, walk-forward evaluation, experiment tracking with MLflow, and a lightweight **FastAPI + React dashboard** for inspection and demos.

## Purpose

Bookforge combines **systems engineering** and **market-microstructure research** in one repository.

Most portfolio projects lean heavily toward either:

- Machine learning without strong systems depth
- Systems code without a research workflow built on top of it

Bookforge bridges that gap with:

- A replayable order-book and matching-engine core
- Reproducible snapshot and feature export
- A strategy-experiment framework for comparing execution approaches under replay
- A Python research workflow for modeling and evaluation
- An API/dashboard layer for inspection and demos

This makes it useful both as:

- A serious **quant SWE / quant research portfolio project**
- A practical sandbox for **short-horizon microstructure and execution experiments**

## Highlights

- C++20 matching engine and price-time-priority order book
- Deterministic replay pipeline for external market-event playback
- Isolated matching-engine books for each replayed symbol
- Optional per-symbol replay filtering with deterministic sorted summaries
- Event-time replay pacing with a configurable speed multiplier
- Hyperliquid-style CSV ingestion path for replay experiments
- Optional external order-ID preservation for lifecycle-aware replay datasets
- Snapshot export and comparison for reproducibility and checkpoint validation
- Feature export for microstructure research, including spread, mid-price, depth imbalance, and OFI
- Strategy-experiment runner, injected-order support, passive/aggressive comparison, and CSV-result writer
- Experiment-result schema covering requested, filled, and remaining quantity; fill rate; average execution price; decision-time metrics; implementation shortfall; and time-to-fill fields
- Python dataset and modeling layer for training short-horizon predictive baselines
- Walk-forward evaluation, feature-importance export, optional SHAP analysis, and MLflow tracking
- FastAPI backend and React/Vite dashboard for replay summaries and feature samples
- C++ and Python benchmarks/tests that keep the core engine and replay pipeline honest

## Core areas

### C++ core

- C++20 matching engine
- Price-time-priority order book
- Replay runner and replay-adapter architecture
- Independent matching-engine instances for symbol-bearing replay data
- Optional event-time pacing with deterministic requested-delay calculations
- Hyperliquid-style CSV reader for external order-event data
- Snapshot builder, serializer, deserializer, and comparator
- Feature-extraction pipeline
- Strategy-experiment configuration, injected-order helpers, adapter, runner, comparison runner, and CSV writer
- GoogleTest coverage for core engine, replay, snapshot, feature, and strategy-experiment logic
- Google Benchmark coverage for order-book hot paths and replay throughput

### Multi-symbol replay

Bookforge routes each symbol-bearing replay event to an independent matching engine and adapter. This prevents orders from one instrument from interacting with liquidity in another instrument, even when they share the same price.

When replaying a CSV with multiple symbols:

- Each symbol receives an isolated order book and matching engine.
- Cross-symbol orders cannot produce trades.
- Final symbol summaries are printed in sorted symbol order.
- `--symbol <symbol>` filters the input before replay, so pacing, metrics, and final-book output apply only to the selected instrument.
- CSV rows without a symbol are routed to the configured fallback symbol, currently `BTCUSDT.P`, preserving compatibility with legacy symbol-less input files.

### Replay pacing

Replay remains **unpaced by default**, preserving fastest-possible event processing for benchmarks and normal test runs.

When event-time pacing is enabled, Bookforge calculates the non-negative timestamp delta between consecutive processed replay events and requests a scaled delay:

\[
\text{requested delay} =
\frac{\max(0,\ t_i - t_{i-1})}{\text{replay speed}}
\]

- The first processed event does not wait.
- `start_offset` establishes a new first-event timing baseline.
- Non-monotonic timestamps request no negative delay.
- A positive speed multiplier accelerates replay; for example, `10` replays timestamp gaps at 10x speed.
- Injected orders retain their ordering around each external event: pacing, `BeforeEvent` orders, external event, then `AfterEvent` orders.

### Strategy experiments

The strategy-experiment layer supports deterministic execution analysis over the replay pipeline.

It currently provides:

- Passive and aggressive strategy modes
- Explicit passive/aggressive comparison configurations against the same immutable replay-event vector and entry offset
- Configurable entry offset, side, limit price, quantity, and injection timing
- Injected-order fill linkage from matching-engine trades into experiment results
- Decision-time top-of-book capture immediately before injected-order submission
- CSV result export with a stable, tested schema
- Sign-aware implementation shortfall in basis points
- A CLI for loading CSV events, filtering a symbol, running an experiment, and writing a result row

The result schema includes:

- `requested_qty`, `filled_qty`, `remaining_qty`, and `fill_rate`
- `avg_execution_price`
- `decision_mid_price`, `decision_spread`, and `has_decision_metrics`
- `implementation_shortfall_bps`
- `time_to_first_fill_us` and `time_to_full_fill_us`

Timestamp-based fill timing remains deferred because current matching-engine timestamps are synthetic sequence values rather than normalized replay-time microseconds.

### Python research layer

- Feature CSV loading and validation
- Dataset-construction utilities
- Label generation
- Baseline model training with XGBoost
- Chronological holdout evaluation
- Walk-forward validation
- Feature-importance export
- Optional SHAP analysis
- MLflow experiment tracking
- Pytest coverage for Python-side utilities and API behavior

### Demo layer

- FastAPI service for replay inspection
- Replay-summary endpoint
- Feature-sample retrieval endpoint
- React dashboard with charts for:
  - Spread
  - Mid-price
  - L1 bid/ask depth
  - Depth imbalance
- Docker Compose support for local demo startup

## Benchmarking

Bookforge includes a microbenchmark for order-book hot paths and a replay benchmark for end-to-end event processing.

The replay benchmark uses a larger synthetic CSV fixture so throughput numbers are meaningful rather than dominated by benchmark overhead. On the current large fixture, the replay benchmark reports roughly **4.4M–4.7M events/sec** in Release mode on Windows, which is a useful baseline for future changes.

Run throughput benchmarks with default unpaced replay. Event-time pacing intentionally includes waiting and is therefore not a throughput benchmark mode.

## Why it matters

Bookforge is meant to demonstrate the kind of end-to-end thinking that shows up in quant and market-data engineering work:

- Building a deterministic systems core
- Validating it with repeatable tests
- Exporting structured state for downstream analysis
- Turning that output into a research and demo workflow

In practice, the repository can be used to:

- Study replayed order-book behavior
- Prototype microstructure features
- Build short-horizon predictive datasets
- Evaluate modeling ideas with chronological discipline
- Develop execution-analysis experiments under replay
- Present outputs through a lightweight interface instead of raw files alone

## Tech stack

### Core systems

- C++20
- CMake
- GoogleTest
- Google Benchmark
- clang-format

### Python / data tooling

- Python 3.11+
- pandas
- numpy
- scipy
- pybind11
- scikit-learn
- xgboost
- shap
- mlflow
- pytest
- Ruff

### API / app layer

- FastAPI
- Pydantic
- Uvicorn
- React
- Vite
- Recharts
- Docker
- Docker Compose

## Repository structure

```text
Bookforge/
├── src/                  # C++ engine, replay, snapshot, feature, and experiment code
├── python/               # Python package, ML scripts, and FastAPI backend
├── dashboard/            # React + Vite frontend
├── tests/                # C++ and Python tests plus fixtures
├── data/                 # Sample and processed datasets
├── docs/                 # Blueprint, progress, architecture, and notes
├── output/               # Generated artifacts such as features and reports
└── build/                # Local CMake build directory
```

## Quick start

### 1. Clone the repo

```bash
git clone <your-repo-url>
cd Bookforge
```

### 2. Create and activate a virtual environment

#### Windows PowerShell

```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
```

#### macOS / Linux

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 3. Install Python dependencies

```bash
pip install -r requirements.txt
```

### 4. Configure and build the C++ project

```bash
cmake -S . -B build
cmake --build build --config Debug
```

## Running tests

### C++ tests

```bash
ctest --test-dir build -C Debug --output-on-failure
```

### Python tests

#### Windows PowerShell

```powershell
$env:PYTHONPATH = "python"
python -m pytest tests/python -q
```

#### macOS / Linux

```bash
PYTHONPATH=python python -m pytest tests/python -q
```

## Usage

The repository includes `data/btc_orders_sample_2025-12-15-12.csv`, a 100,000-row Hyperliquid-style BTC order-status sample. Its schema is symbol-less:

```text
ts,limitPx,sz,isAsk,statusId
```

Bookforge routes symbol-less rows to the fallback symbol `BTCUSDT.P`. The sample is useful for validating ingestion, replay CLI behavior, fallback symbol routing, and experiment result export. Its status-oriented events may not create active resting liquidity in the current replay adapter, so strategy experiments against this sample can complete with zero fills and unavailable decision-book metrics.

### Optional external order IDs

For lifecycle-aware replay datasets, the CSV reader preserves an optional external order identifier when one of these header names is present:

```text
order_id
orderId
oid
```

The checked-in BTC status sample has no external ID column, so its parsed events retain an empty external ID. Bookforge does not infer order identity from price, size, timestamp, or side. Stateful replay handling for external cancels and fills will only operate when a dataset provides an explicit order identifier.

When a `New` event with an explicit external ID rests in the internal book, a later `Cancel` event with the same ID removes that resting order. Unknown IDs, repeated cancels, empty IDs, and orders that fully crossed at submission are safe no-ops. External fill lifecycle events are still recorded but remain unsupported until partial/full-fill linkage is implemented.

### Replay Hyperliquid-style CSV data

#### Windows PowerShell

```powershell
.\build\Debug\hyperliquid_replay_main.exe data\btc_orders_sample_2025-12-15-12.csv
```

#### macOS / Linux

```bash
./build/hyperliquid_replay_main data/btc_orders_sample_2025-12-15-12.csv
```

When a CSV contains multiple symbols, replay prints a separate final-book summary for each symbol in sorted symbol order.

### Replay one symbol

Use `--symbol <symbol>` to replay one instrument from a symbol-bearing CSV.

#### Windows PowerShell

```powershell
.\build\Debug\hyperliquid_replay_main.exe data\btc_orders_sample_2025-12-15-12.csv --symbol BTCUSDT.P
```

#### macOS / Linux

```bash
./build/hyperliquid_replay_main data/btc_orders_sample_2025-12-15-12.csv --symbol BTCUSDT.P
```

The filter is applied before replay. As a result, event-time pacing, replay metrics, trade counts, and final-book summaries represent only the selected symbol.

For legacy CSV files without a symbol column, Bookforge uses the configured fallback symbol, currently `BTCUSDT.P`. Therefore, the commands above include all rows from the checked-in BTC sample. A different symbol filter excludes those symbol-less rows.

### Replay with event-time pacing

By default, replay is unpaced and processes events as quickly as possible.

#### Windows PowerShell

```powershell
# Preserve default fastest-possible replay behavior.
.\build\Debug\hyperliquid_replay_main.exe data\btc_orders_sample_2025-12-15-12.csv --pacing unpaced

# Wait for recorded event-time gaps.
.\build\Debug\hyperliquid_replay_main.exe data\btc_orders_sample_2025-12-15-12.csv --pacing event-time

# Replay recorded timestamp gaps at 10x speed.
.\build\Debug\hyperliquid_replay_main.exe data\btc_orders_sample_2025-12-15-12.csv --pacing event-time --speed 10

# Select legacy symbol-less BTC rows and use event-time pacing.
.\build\Debug\hyperliquid_replay_main.exe data\btc_orders_sample_2025-12-15-12.csv --symbol BTCUSDT.P --pacing event-time
```

#### macOS / Linux

```bash
# Preserve default fastest-possible replay behavior.
./build/hyperliquid_replay_main data/btc_orders_sample_2025-12-15-12.csv --pacing unpaced

# Wait for recorded event-time gaps.
./build/hyperliquid_replay_main data/btc_orders_sample_2025-12-15-12.csv --pacing event-time

# Replay recorded timestamp gaps at 10x speed.
./build/hyperliquid_replay_main data/btc_orders_sample_2025-12-15-12.csv --pacing event-time --speed 10

# Select legacy symbol-less BTC rows and use event-time pacing.
./build/hyperliquid_replay_main data/btc_orders_sample_2025-12-15-12.csv --symbol BTCUSDT.P --pacing event-time
```

Supported replay options:

```text
[input_csv]
--symbol <symbol>
--pacing unpaced|event-time
--speed <positive-number>
```

### Export features from replay data

#### Windows PowerShell

```powershell
.\build\Debug\feature_export_main.exe --input data\btc_orders_sample_2025-12-15-12.csv --output output\features.csv --symbol BTCUSDT.P --snapshot-depth 10 --imbalance-depth 10 --ofi-depth 10 --rolling-window 50
```

#### macOS / Linux

```bash
./build/feature_export_main --input data/btc_orders_sample_2025-12-15-12.csv --output output/features.csv --symbol BTCUSDT.P --snapshot-depth 10 --imbalance-depth 10 --ofi-depth 10 --rolling-window 50
```

### Run a strategy experiment

The strategy-experiment executable reads Hyperliquid-style CSV events, applies an optional symbol filter, injects one configured order at the selected event offset, and writes a one-row CSV result containing fill, decision-book, and implementation-shortfall metrics.

`entry-offset` is zero-based and applies after any `--symbol` filtering.

#### Windows PowerShell

```powershell
.\build\Debug\strategy_experiment_main.exe `
    --input data\btc_orders_sample_2025-12-15-12.csv `
    --output output\strategy_experiment_results.csv `
    --symbol BTCUSDT.P `
    --mode aggressive `
    --side buy `
    --limit-price 100000 `
    --quantity 1 `
    --entry-offset 0
```

#### macOS / Linux

```bash
./build/strategy_experiment_main \
    --input data/btc_orders_sample_2025-12-15-12.csv \
    --output output/strategy_experiment_results.csv \
    --symbol BTCUSDT.P \
    --mode aggressive \
    --side buy \
    --limit-price 100000 \
    --quantity 1 \
    --entry-offset 0
```

Use `--symbol <symbol>` to isolate an experiment to one instrument in a multi-symbol CSV. If omitted, all events are replayed. For the checked-in symbol-less BTC sample, `--symbol BTCUSDT.P` includes all rows through fallback routing.

The current `passive` and `aggressive` modes are recorded in the output result. Set `--limit-price` explicitly to control whether the injected order rests or crosses available liquidity.

Supported options:

```text
--input <csv>
--output <csv>
--symbol <symbol>
--mode passive|aggressive
--side buy|sell
--limit-price <positive-number>
--quantity <positive-integer>
--entry-offset <zero-based-event-index>
```

### Multi-symbol fixture demo

The repository includes a small deterministic fixture at:

```text
tests/fixtures/hyperliquid_multi_symbol_fixture.csv
```

It contains interleaved BTC and ETH `New` events with independent books. Replay all symbols:

```powershell
.\build\Debug\hyperliquid_replay_main.exe tests\fixtures\hyperliquid_multi_symbol_fixture.csv
```

Expected final top-of-book levels:

```text
BTCUSDT.P: best bid 99.0, best ask 100.0
ETHUSDT.P: best bid 89.0, best ask 90.0
```

Replay BTC only:

```powershell
.\build\Debug\hyperliquid_replay_main.exe tests\fixtures\hyperliquid_multi_symbol_fixture.csv --symbol BTCUSDT.P
```

Run a BTC experiment against only BTC liquidity:

```powershell
.\build\Debug\strategy_experiment_main.exe `
    --input tests\fixtures\hyperliquid_multi_symbol_fixture.csv `
    --output output\btc_fixture_experiment.csv `
    --symbol BTCUSDT.P `
    --mode aggressive `
    --side buy `
    --limit-price 101 `
    --quantity 2 `
    --entry-offset 1
```

The fixture demonstrates that BTC and ETH liquidity remain isolated. Use it to validate sorted per-symbol replay summaries and `--symbol` filtering. The current fixture quantities are intentionally small and primarily support order-book regression coverage rather than a whole-unit strategy fill demonstration.

### Benchmark replay throughput

#### Windows PowerShell

```powershell
.\build\bench\Release\benchmark_replay.exe
```

#### macOS / Linux

```bash
./build/bench/benchmark_replay
```

### Train a baseline model

#### Windows PowerShell

```powershell
$env:PYTHONPATH = "python"
python python/ml/train.py --features-csv output\features.csv --label-type classification --horizon-events 50 --up-threshold 0.0 --down-threshold 0.0
```

#### macOS / Linux

```bash
PYTHONPATH=python python/ml/train.py --features-csv output/features.csv --label-type classification --horizon-events 50 --up-threshold 0.0 --down-threshold 0.0
```

### Run walk-forward evaluation with MLflow

#### Windows PowerShell

```powershell
$env:PYTHONPATH = "python"
$env:MLFLOW_TRACKING_URI = "sqlite:///mlruns.db"
python python/ml/train.py --features-csv output\features.csv --label-type classification --horizon-events 50 --up-threshold 0.0 --down-threshold 0.0 --validation walk_forward --wf-initial-train-size 50000 --wf-test-size 10000 --wf-step-size 10000 --wf-max-folds 5 --enable-mlflow --mlflow-experiment bookforge --enable-shap --shap-sample-size 2000
```

#### macOS / Linux

```bash
PYTHONPATH=python MLFLOW_TRACKING_URI=sqlite:///mlruns.db python python/ml/train.py --features-csv output/features.csv --label-type classification --horizon-events 50 --up-threshold 0.0 --down-threshold 0.0 --validation walk_forward --wf-initial-train-size 50000 --wf-test-size 10000 --wf-step-size 10000 --wf-max-folds 5 --enable-mlflow --mlflow-experiment bookforge --enable-shap --shap-sample-size 2000
```

### Launch the API

#### Windows PowerShell

```powershell
$env:PYTHONPATH = "python"
uvicorn api.main:app --reload --port 8010
```

#### macOS / Linux

```bash
PYTHONPATH=python uvicorn api.main:app --reload --port 8010
```

### Launch the dashboard

```bash
cd dashboard
npm install
npm run dev
```

### Run the local demo with Docker Compose

```bash
docker compose up --build
```

### Generate a replay fixture

#### Windows PowerShell

```powershell
python tools\generate_replay_fixture.py --events 10000 --base-price 100.00 --output tests\fixtures\hyperliquid_replay_fixture_large.csv
```

#### macOS / Linux

```bash
python tools/generate_replay_fixture.py --events 10000 --base-price 100.00 --output tests/fixtures/hyperliquid_replay_fixture_large.csv
```

### Generate a synthetic replay fixture

#### Windows PowerShell

```powershell
python tools\generate_synthetic_market_events.py --events 10000 --base-price 100000 --tick-size 0.5 --base-spread-ticks 2 --min-size 0.001 --max-size 0.05 --seed 42 --output data\synthetic_replay_fixture.csv
```

#### macOS / Linux

```bash
python tools/generate_synthetic_market_events.py --events 10000 --base-price 100000 --tick-size 0.5 --base-spread-ticks 2 --min-size 0.001 --max-size 0.05 --seed 42 --output data/synthetic_replay_fixture.csv
```

## Formatting

### Python

```bash
ruff check .
ruff format .
```

### C++

The repository uses a root `.clang-format` file and checks formatting in CI.

### Local development helper

For a one-command local check, use the PowerShell helper:

```powershell
.\scripts\dev-check.ps1
```

This formats C++ source/header files under `src`, `tests`, and `bench`, then runs the CMake build and CTest suite.

The repository also uses `.gitattributes` to keep line endings consistent across platforms.

## Limitations

- This repo is educational and research-oriented.
- It is **not** a production trading system.
- The current Hyperliquid replay path is still an approximation of full lifecycle behavior.
- External/internal cancel and fill linkage is still evolving.
- Matching-engine timestamps used by the current replay adapter are synthetic sequence values; they are not yet suitable for time-to-fill measurements.
- Event-time pacing uses input event timestamps and wall-clock sleeping, so it is intended for controlled replay behavior rather than maximum throughput.
- The checked-in BTC order-status sample is symbol-less and may not generate active resting liquidity under the current event-status mapping.
- The baseline ML pipeline works, but label quality and class balance remain active research problems.

## Additional documentation

Detailed planning and implementation progress live in:

- `docs/BLUEPRINT.md`
- `docs/PROGRESS.md`

Additional design and reference material lives in:

- `docs/ARCHITECTURE.md`
- `docs/DATA_GUIDE.md`
- `docs/SNAPSHOT_SCHEMA.md`
- `docs/INTERVIEW_PREP.md`
- `docs/WEEK_BY_WEEK.md`
- `docs/BENCHMARKS.md`
- `docs/adr/` — architecture decision records for major design choices

## Author

Bookforge is developed and maintained by:

- **Dong Quan Tran (Johnny)**
- Email: [dxt9721@mavs.uta.edu](mailto:dxt9721@mavs.uta.edu) / [dongquan.tran.johnny@gmail.com](mailto:dongquan.tran.johnny@gmail.com)
- GitHub: [dong-quan-tran](https://github.com/dong-quan-tran)

