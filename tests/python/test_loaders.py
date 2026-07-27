import numpy as np
import pandas as pd
import pytest

from python.bookforge_py.loaders import (
    feature_column_names,
    split_feature_columns,
    validate_feature_frame,
)


def _make_valid_frame(n: int = 5) -> pd.DataFrame:
    replay_event_index = np.arange(n, dtype=np.int64)
    replay_timestamp_ns = np.arange(n, dtype=np.int64) * 1_000_000
    best_bid = np.full(n, 100.0)
    best_ask = np.full(n, 101.0)
    spread = best_ask - best_bid
    mid_price = (best_bid + best_ask) / 2.0

    df = pd.DataFrame(
        {
            "replay_event_index": replay_event_index,
            "replay_timestamp_ns": replay_timestamp_ns,
            "best_bid": best_bid,
            "best_ask": best_ask,
            "spread": spread,
            "mid_price": mid_price,
            "symbol": ["BTCUSDT.P"] * n,
            "l1_bid_qty": np.ones(n),
            "l1_ask_qty": np.ones(n),
        }
    )
    return df


def test_validate_feature_frame_happy_path():
    df = _make_valid_frame()
    # should not raise
    validate_feature_frame(df)


def test_validate_feature_frame_missing_required_columns_raises():
    df = _make_valid_frame().drop(columns=["best_bid"])
    with pytest.raises(ValueError) as excinfo:
        validate_feature_frame(df)
    assert "Missing required columns" in str(excinfo.value)


def test_validate_feature_frame_non_monotonic_indices_allowed_for_now():
    df = _make_valid_frame()
    df.loc[2, "replay_event_index"] = df.loc[1, "replay_event_index"]
    # current implementation does not raise; this test just documents that behavior
    validate_feature_frame(df)


def test_validate_feature_frame_spread_relation_enforced():
    df = _make_valid_frame()
    df.loc[0, "spread"] = df.loc[0, "spread"] + 1.0
    with pytest.raises(ValueError) as excinfo:
        validate_feature_frame(df)
    assert "spread must approximately equal best_ask - best_bid" in str(excinfo.value)


def test_validate_feature_frame_mid_price_relation_enforced():
    df = _make_valid_frame()
    df.loc[0, "mid_price"] = df.loc[0, "mid_price"] + 1.0
    with pytest.raises(ValueError) as excinfo:
        validate_feature_frame(df)
    assert "mid_price must approximately equal (best_bid + best_ask) / 2" in str(excinfo.value)


def test_non_negative_columns_enforced():
    df = _make_valid_frame()
    df["rolling_mean_spread"] = np.ones(len(df))
    df.loc[0, "rolling_mean_spread"] = -0.1
    with pytest.raises(ValueError) as excinfo:
        validate_feature_frame(df)
    assert "rolling_mean_spread must be non-negative" in str(excinfo.value)


def test_split_feature_columns_separates_metadata_and_features():
    df = _make_valid_frame()
    meta, feats = split_feature_columns(df)
    # default metadata columns should be in meta, not in feats
    assert set(meta.columns) == {"replay_event_index", "replay_timestamp_ns"}
    assert "replay_event_index" not in feats.columns
    assert "replay_timestamp_ns" not in feats.columns


def test_feature_column_names_excludes_metadata():
    df = _make_valid_frame()
    feature_cols = feature_column_names(df)
    assert "replay_event_index" not in feature_cols
    assert "replay_timestamp_ns" not in feature_cols
    # some known feature columns should be present
    for col in ["best_bid", "best_ask", "spread", "mid_price"]:
        assert col in feature_cols
