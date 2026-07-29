# Bookforge

Bookforge is a hybrid **C++ + Python** market microstructure project for studying how a modern limit order book behaves under replayed market event flow.

At its core is a low-latency **C++20 matching engine and price-time-priority order book** with deterministic replay infrastructure, snapshot export, feature export, and regression-tested historical event playback. On top of that, the project includes a Python research layer for dataset construction, short-horizon machine learning, walk-forward evaluation, experiment tracking with MLflow, and a lightweight **FastAPI + React dashboard** for inspection and demos.

## Purpose

Bookforge is designed to combine **systems engineering** and **market microstructure research** in one repository.

Most portfolio projects lean heavily toward either:
- machine learning without strong systems depth, or
- systems code without a research workflow built on top of it.

Bookforge aims to bridge that gap by providing:
- a replayable order-book and matching-engine core,
- reproducible snapshot and feature export,
- a Python research workflow for modeling and evaluation,
- and an API/dashboard layer for inspection and demos.

This makes it useful both as:
- a serious **quant SWE / quant research portfolio project**, and
- a practical sandbox for **short-horizon microstructure experiments**.

## What it does

Bookforge currently supports:

- A **price-time-priority order book** with FIFO behavior at each price level.
- A **matching engine** that handles passive insertion, aggressive matching, partial fills, and multi-level sweeps.
- A **deterministic replay pipeline** for external market-event playback.
- A **Hyperliquid-style CSV ingestion path** for replay experiments.
- **Snapshot export and comparison** for reproducibility and checkpoint validation.
- **Feature export** for microstructure research, including spread, mid-price, depth imbalance, and OFI.
- A **Python dataset and modeling layer** for training short-horizon predictive baselines.
- **Walk-forward evaluation**, **feature importance**, optional **SHAP analysis**, and **MLflow tracking**.
- A **FastAPI backend** and **React/Vite dashboard** for viewing replay summaries and feature samples.
- **C++ and Python benchmarks/tests** that keep the core engine and replay pipeline honest.

## Key features

### C++ core
- C++20 matching engine
- Price-time-priority order book
- Replay runner and replay adapter architecture
- Snapshot builder, serializer, deserializer, and comparator
- Feature extraction pipeline
- GoogleTest coverage for core engine, replay, snapshot, and feature logic
- Google Benchmark coverage for order-book hot paths and replay throughput

### Python research layer
- Feature CSV loading and validation
- Dataset construction utilities
- Label generation
- Baseline model training with XGBoost
- Chronological holdout evaluation
- Walk-forward validation
- Feature importance export
- Optional SHAP analysis
- MLflow experiment tracking
- Pytest coverage for Python-side utilities and API behavior

### Demo layer
- FastAPI service for replay inspection
- Replay summary endpoint
- Feature sample retrieval endpoint
- React dashboard with charts for:
  - spread
  - mid-price
  - L1 bid/ask depth
  - depth imbalance
- Docker Compose support for local demo startup

## Benchmarking

Bookforge includes a microbenchmark for order-book hot paths and a replay benchmark for end-to-end event processing.

The replay benchmark uses a larger synthetic CSV fixture so throughput numbers are meaningful rather than dominated by benchmark overhead. On the current large fixture, the replay benchmark reports roughly **4.4M–4.7M events/sec** in Release mode on Windows, which is a useful baseline for future changes.

## Why it matters

Bookforge is meant to demonstrate the kind of end-to-end thinking that shows up in quant and market-data engineering work:

- building a deterministic systems core,
- validating it with repeatable tests,
- exporting structured state for downstream analysis,
- and turning that output into a research and demo workflow.

In practice, that means the repo can be used to:
- study replayed order-book behavior,
- prototype microstructure features,
- build short-horizon predictive datasets,
- evaluate simple modeling ideas with chronological discipline,
- and present outputs through a lightweight interface instead of raw files alone.

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
├── src/                 # C++ engine, replay, snapshot, and feature code
├── python/              # Python package, ML scripts, and FastAPI backend
├── dashboard/           # React + Vite frontend
├── tests/               # C++ and Python tests plus fixtures
├── data/                # sample and processed datasets
├── docs/                # blueprint, progress, architecture, and notes
├── output/              # generated artifacts such as features and reports
└── build/               # local CMake build directory
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
PYTHONPATH=python python python/ml/train.py --features-csv output/features.csv --label-type classification --horizon-events 50 --up-threshold 0.0 --down-threshold 0.0
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

## Formatting

### Python
```bash
ruff check .
ruff format .
```

### C++
The repository uses a root `.clang-format` file and checks formatting in CI.

## Limitations

- This repo is educational and research-oriented.
- It is **not** a production trading system.
- The current Hyperliquid replay path is still an approximation of full lifecycle behavior.
- External/internal cancel and fill linkage is still evolving.
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

## Author

Bookforge is developed and maintained by:

- **Dong Quan Tran (Johnny)**
- Email: [dxt9721@mavs.uta.edu](mailto:dxt9721@mavs.uta.edu) / [dongquan.tran.johnny@gmail.com](mailto:dongquan.tran.johnny@gmail.com)
- GitHub: [dong-quan-tran](https://github.com/dong-quan-tran)