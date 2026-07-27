from pathlib import Path

import pandas as pd
from fastapi.testclient import TestClient

from python.api.main import app

client = TestClient(app)


def _write_features_csv(tmp_path: Path) -> Path:
    df = pd.DataFrame(
        {
            "replay_event_index": [0, 1, 2],
            "replay_timestamp_ns": [100, 200, 300],
            "symbol": ["BTCUSDT.P", "BTCUSDT.P", "BTCUSDT.P"],
            "spread": [1.0, 1.5, 1.25],
            "mid_price": [100.0, 100.5, 100.25],
            "depth_imbalance": [0.1, -0.2, 0.05],
            "bid_depth": [10.0, 11.0, 9.5],
            "ask_depth": [9.0, 10.0, 9.75],
        }
    )
    path = tmp_path / "features.csv"
    df.to_csv(path, index=False)
    return path


def _write_snapshots_csv(tmp_path: Path) -> Path:
    df = pd.DataFrame(
        {
            "replay_event_index": [0, 1],
            "replay_timestamp_ns": [100, 200],
            "symbol": ["BTCUSDT.P", "BTCUSDT.P"],
            "best_bid": [99.5, 99.75],
            "best_ask": [100.5, 100.75],
            "mid_price": [100.0, 100.25],
            "spread": [1.0, 1.0],
            "bid_price_1": [99.5, 99.75],
            "bid_size_1": [10.0, 11.0],
            "bid_price_2": [99.0, 99.25],
            "bid_size_2": [8.0, 9.0],
            "ask_price_1": [100.5, 100.75],
            "ask_size_1": [9.0, 10.0],
            "ask_price_2": [101.0, 101.25],
            "ask_size_2": [7.0, 8.0],
        }
    )
    path = tmp_path / "snapshots.csv"
    df.to_csv(path, index=False)
    return path


def test_health():
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json()["status"] == "ok"


def test_replay_summary(tmp_path: Path):
    csv_path = _write_features_csv(tmp_path)
    response = client.get("/api/replay/summary", params={"features_csv": str(csv_path)})
    assert response.status_code == 200
    body = response.json()
    assert body["row_count"] == 3
    assert body["symbol"] == "BTCUSDT.P"
    assert "spread" in body["columns"]


def test_feature_sample(tmp_path: Path):
    csv_path = _write_features_csv(tmp_path)
    response = client.get(
        "/api/features/sample",
        params={"features_csv": str(csv_path), "limit": 2, "offset": 1},
    )
    assert response.status_code == 200
    body = response.json()
    assert body["returned_count"] == 2
    assert body["rows"][0]["row_index"] == 1


def test_feature_columns(tmp_path: Path):
    csv_path = _write_features_csv(tmp_path)
    response = client.get("/api/features/columns", params={"features_csv": str(csv_path)})
    assert response.status_code == 200
    body = response.json()
    assert "mid_price" in body["columns"]


def test_snapshot_sample(tmp_path: Path):
    csv_path = _write_snapshots_csv(tmp_path)
    response = client.get(
        "/api/snapshots/sample",
        params={"snapshot_csv": str(csv_path), "limit": 1},
    )
    assert response.status_code == 200
    body = response.json()
    assert body["returned_count"] == 1
    assert "best_bid" in body["rows"][0]["values"]


def test_snapshot_inspect(tmp_path: Path):
    csv_path = _write_snapshots_csv(tmp_path)
    response = client.get(
        "/api/snapshots/inspect",
        params={"snapshot_csv": str(csv_path), "row_index": 1},
    )
    assert response.status_code == 200
    body = response.json()
    assert body["row_index"] == 1
    assert body["symbol"] == "BTCUSDT.P"
    assert body["best_bid"] == 99.75
    assert body["best_ask"] == 100.75
    assert body["mid_price"] == 100.25
    assert len(body["bids"]) == 2
    assert len(body["asks"]) == 2
    assert body["bids"][0]["price"] == 99.75
    assert body["bids"][0]["size"] == 11.0
    assert body["asks"][0]["price"] == 100.75
    assert body["asks"][0]["size"] == 10.0


def test_missing_file_returns_404(tmp_path: Path):
    missing = tmp_path / "missing.csv"
    response = client.get("/api/replay/summary", params={"features_csv": str(missing)})
    assert response.status_code == 404
