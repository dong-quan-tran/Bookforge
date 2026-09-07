import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
import pandas as pd


def _write_feature_csv(path: Path, rows: int = 160) -> None:
    event_index = np.arange(rows)
    mid = 100.0 + np.cumsum(np.where(event_index % 2 == 0, 0.05, -0.03))
    bid = mid - 0.05
    ask = mid + 0.05

    df = pd.DataFrame(
        {
            "replay_event_index": event_index,
            "replay_timestamp_ns": event_index * 1_000_000,
            "symbol": ["BTCUSDT.P"] * rows,
            "best_bid": bid,
            "best_ask": ask,
            "spread": ask - bid,
            "mid_price": mid,
            "l1_bid_qty": 100.0 + (event_index % 7),
            "l1_ask_qty": 90.0 + ((event_index * 3) % 7),
            "l1_depth_imbalance": np.sin(event_index / 5.0),
            "ofi_l1": np.cos(event_index / 4.0),
        }
    )

    df.to_csv(path, index=False)


def test_train_cli_writes_holdout_artifacts(tmp_path: Path):
    features_csv = tmp_path / "features.csv"
    output_dir = tmp_path / "artifacts"
    _write_feature_csv(features_csv)

    repo_root = Path(__file__).resolve().parents[2]
    train_script = repo_root / "python" / "ml" / "train.py"

    env = os.environ.copy()
    python_path = str(repo_root / "python")
    existing_python_path = env.get("PYTHONPATH")
    env["PYTHONPATH"] = (
        python_path
        if not existing_python_path
        else f"{python_path}{os.pathsep}{existing_python_path}"
    )

    result = subprocess.run(
        [
            sys.executable,
            str(train_script),
            "--features-csv",
            str(features_csv),
            "--label-type",
            "classification",
            "--horizon-events",
            "1",
            "--purge-events",
            "1",
            "--train-fraction",
            "0.7",
            "--output-dir",
            str(output_dir),
        ],
        check=False,
        capture_output=True,
        text=True,
        cwd=repo_root,
        env=env,
    )

    assert result.returncode == 0, (
        f"train.py failed with exit code {result.returncode}\n"
        f"stdout:\n{result.stdout}\n"
        f"stderr:\n{result.stderr}"
    )

    metrics_path = output_dir / "metrics.json"
    model_path = output_dir / "xgboost_baseline.json"
    feature_columns_path = output_dir / "feature_columns.json"
    importance_path = output_dir / "feature_importance.csv"

    assert result.stdout
    assert metrics_path.exists()
    assert model_path.exists()
    assert feature_columns_path.exists()
    assert importance_path.exists()

    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))

    assert metrics["task"] == "classification"
    assert metrics["validation"] == "holdout"
    assert metrics["purge_events"] == 1
    assert metrics["n_train"] > 0
    assert metrics["n_test"] > 0
    assert "accuracy" in metrics
    assert "train_class_balance" in metrics
    assert "test_class_balance" in metrics
