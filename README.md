# Bookforge

Bookforge is a hybrid **C++ + Python** market-microstructure project for studying how a modern limit-order book behaves under replayed market-event flow.

At its core is a low-latency **C++20 matching engine** with a price-time-priority order book, deterministic replay infrastructure, snapshot export, feature export, strategy-experiment scaffolding, and regression-tested historical event playback. On top of that, the repository includes a Python research layer for dataset construction, short-horizon machine learning, walk-forward evaluation, experiment tracking with MLflow, and a lightweight **FastAPI + React dashboard** for inspection and demos.

## Purpose

Bookforge combines **systems engineering** and **market-microstructure research** in one repository.

Most portfolio projects lean heavily toward either:

- Machine learning without strong systems depth
- Systems code without a research workflow built on top of it

Bookforge bridges that gap with:

- A replayable order-book and matching-engine core
- Reproducible snapshot and feature export
- A strategy-experiment scaffold for comparing execution approaches under replay
- A Python research workflow for modeling and evaluation
- An API/dashboard layer for inspection and demos

This makes it useful both as:

- A serious **quant SWE / quant research portfolio project**
- A practical sandbox for **short-horizon microstructure and execution experiments**

## Highlights

- C++20 matching engine and price-time-priority order book
- Deterministic replay pipeline for external market-event playback
- Hyperliquid-style CSV ingestion path for replay experiments
- Snapshot export and comparison for reproducibility and checkpoint validation
- Feature export for microstructure research, including spread, mid-price, depth imbalance, and OFI
- Strategy-experiment CLI, runner, adapter, and CSV-result writer for passive/aggressive experiment scaffolding
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
- Hyperliquid-style CSV reader for external order-event data
- Snapshot builder, serializer, deserializer, and comparator
- Feature-extraction pipeline
- Strategy-experiment configuration, injected-order helpers, adapter, runner, CLI, and CSV writer
- GoogleTest coverage for core engine, replay, snapshot, feature, and strategy-experiment logic
- Google Benchmark coverage for order-book hot paths and replay throughput

### Strategy experiments

The strategy-experiment layer is an in-progress execution-analysis harness built around replayed event flow.

It currently provides:

- Passive and aggressive strategy modes
- Configurable entry offset, side, limit price, quantity, and injection timing
- A replay adapter that initializes decision-time and execution-quality result fields
- A runner scaffold that feeds replay events through the adapter
- CSV result export with a stable, tested schema
- A CLI entry point for parsing experiment configuration

The result schema includes:

- `requested_qty`, `filled_qty`, `remaining_qty`, and `fill_rate`
- `avg_execution_price`
- `decision_mid_price`, `decision_spread`, and `has_decision_metrics`
- `implementation_shortfall_bps`
- `time_to_first_fill_us` and `time_to_full_fill_us`

The current harness is intentionally incremental. Real injected-order fill linkage, full decision-time order-book capture, timestamp-based fill timing, and implementation-shortfall calculation are still being wired into the replay path.

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

### Export features from replay data

#### Windows PowerShell

```powershell
.\build\Debug\feature_export_main.exe --input data\btc_orders_sample_2025-12-15-12.csv --output output\features.csv --symbol BTCUSDT.P --snapshot-depth 10 --imbalance-depth 10 --ofi-depth 10 --rolling-window 50
```

#### macOS / Linux

```bash
./build/feature_export_main --input data/btc_orders_sample_2025-12-15-12.csv --output output/features.csv --symbol BTCUSDT.P --snapshot-depth 10 --imbalance-depth 10 --ofi-depth 10 --rolling-window 50
```

### Inspect strategy-experiment CLI options

The strategy-experiment executable currently parses and displays experiment configuration while replay integration continues to evolve.

#### Windows PowerShell

```powershell
.\build\Debug\strategy_experiment_main.exe --input data\btc_orders_sample_2025-12-15-12.csv --mode passive --side buy --quantity 1 --entry-offset 0
```

#### macOS / Linux

```bash
./build/strategy_experiment_main --input data/btc_orders_sample_2025-12-15-12.csv --mode passive --side buy --quantity 1 --entry-offset 0
```

Supported options:

```text
--input <csv>
--output <csv>
--mode passive|aggressive
--side buy|sell
--quantity <N>
--entry-offset <N>
```

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
.\scripts\dev-check.ps1 -Files src\replay\StrategyExperiment.hpp,src\replay\StrategyExperiment.cpp,src\replay\StrategyExperimentAdapter.hpp,src\replay\StrategyExperimentAdapter.cpp,src\replay\StrategyExperimentRunner.cpp,src\replay\StrategyExperimentCsvWriter.hpp,src\replay\StrategyExperimentCsvWriter.cpp,tests\cpp\test_strategy_experiment.cpp,tests\cpp\test_strategy_experiment_adapter.cpp,tests\cpp\test_strategy_experiment_runner.cpp,tests\cpp\test_strategy_experiment_csv_writer.cpp
```

This formats the listed files, then runs the CMake build and CTest suite.

The repository also uses `.gitattributes` to keep line endings consistent across platforms.

## Limitations

- This repo is educational and research-oriented.
- It is **not** a production trading system.
- The current Hyperliquid replay path is still an approximation of full lifecycle behavior.
- External/internal cancel and fill linkage is still evolving.
- The strategy-experiment layer currently provides tested configuration, result-schema, adapter, runner, CLI, and CSV-export plumbing; real fill linkage and execution-quality calculations remain in progress.
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