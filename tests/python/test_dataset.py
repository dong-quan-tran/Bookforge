import numpy as np
import pandas as pd
from bookforge_py.dataset import (
    TrainingDataset,
    build_training_dataset_from_frame,
    chronological_split,
    walk_forward_splits,
)


def _make_base_frame(n: int = 100) -> pd.DataFrame:
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
        }
    )
    return df


def _make_labeled_frame(n: int = 100) -> pd.DataFrame:
    df = _make_base_frame(n)
    # simple synthetic target: alternating -1 and 1
    df["target"] = np.where(df["replay_event_index"] % 2 == 0, -1, 1)
    return df


def test_build_training_dataset_from_frame_classification_normalizes_labels():
    df = _make_labeled_frame(10)
    dataset: TrainingDataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    # labels should be 0-based contiguous integers
    unique_labels = sorted(dataset.y.unique().tolist())

    assert all(isinstance(v, (int, np.integer)) for v in unique_labels)

    if len(unique_labels) > 1:
        assert unique_labels[0] == 0
        assert unique_labels[-1] == len(unique_labels) - 1


def test_build_training_dataset_from_frame_shapes_consistent():
    df = _make_labeled_frame(50)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    assert len(dataset.X) == len(dataset.y) == len(dataset.metadata) == len(dataset.full_frame)
    # basic column sanity
    assert "replay_event_index" in dataset.metadata.columns
    assert "replay_timestamp_ns" in dataset.metadata.columns
    assert "target" in dataset.full_frame.columns


def test_chronological_split_preserves_order_and_fraction():
    df = _make_labeled_frame(50)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    split = chronological_split(dataset, train_fraction=0.6)
    n_total = len(dataset.X)
    assert len(split.X_train) + len(split.X_test) == n_total

    # train/test windows should be non-overlapping and ordered
    assert (
        split.meta_train["replay_event_index"].iloc[-1]
        < split.meta_test["replay_event_index"].iloc[0]
    )


def test_walk_forward_splits_generates_expected_folds():
    df = _make_labeled_frame(120)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    folds = walk_forward_splits(
        dataset,
        initial_train_size=60,
        test_size=20,
        step_size=20,
        max_folds=2,
    )

    assert len(folds) == 2

    # fold 0: train [0, 60), test [60, 80)
    f0 = folds[0]
    assert f0.train_start == 0
    assert f0.train_end == 60
    assert f0.test_start == 60
    assert f0.test_end == 80
    assert len(f0.X_train) == 60
    assert len(f0.X_test) == 20

    # fold 1: train [0, 80), test [80, 100)
    f1 = folds[1]
    assert f1.train_end == 80
    assert f1.test_start == 80
    assert len(f1.X_train) == 80
    assert len(f1.X_test) == 20

    # all folds should preserve chronological order
    for fold in folds:
        assert fold.meta_train["replay_event_index"].is_monotonic_increasing
        assert fold.meta_test["replay_event_index"].is_monotonic_increasing

import pytest
from bookforge_py.dataset import select_feature_columns


def test_select_feature_columns_excludes_default_non_features():
    df = _make_labeled_frame(10)
    cols = select_feature_columns(df)
    assert "symbol" not in cols
    assert "replay_event_index" not in cols
    assert "replay_timestamp_ns" not in cols
    assert "best_bid" in cols
    assert "best_ask" in cols


def test_select_feature_columns_respects_custom_exclusions():
    df = _make_labeled_frame(10)
    cols = select_feature_columns(df, exclude_columns={"symbol", "spread"})
    assert "symbol" not in cols
    assert "spread" not in cols
    assert "best_bid" in cols


def test_chronological_split_rejects_invalid_fraction_low():
    df = _make_labeled_frame(10)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        chronological_split(dataset, train_fraction=0.0)
    assert "train_fraction must be between 0 and 1" in str(excinfo.value)


def test_chronological_split_rejects_invalid_fraction_high():
    df = _make_labeled_frame(10)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        chronological_split(dataset, train_fraction=1.0)
    assert "train_fraction must be between 0 and 1" in str(excinfo.value)


def test_chronological_split_rejects_too_small_dataset():
    df = _make_labeled_frame(1)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        chronological_split(dataset, train_fraction=0.8)
    assert "Need at least 2 rows to split dataset" in str(excinfo.value)


def test_walk_forward_splits_rejects_small_dataset():
    df = _make_labeled_frame(2)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=1, test_size=1)
    assert "Need at least 3 rows for walk-forward validation" in str(excinfo.value)


def test_walk_forward_splits_rejects_non_positive_initial_train_size():
    df = _make_labeled_frame(10)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=0, test_size=2)
    assert "initial_train_size must be positive" in str(excinfo.value)


def test_walk_forward_splits_rejects_non_positive_test_size():
    df = _make_labeled_frame(10)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=5, test_size=0)
    assert "test_size must be positive" in str(excinfo.value)


def test_walk_forward_splits_rejects_non_positive_step_size():
    df = _make_labeled_frame(10)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=5, test_size=2, step_size=0)
    assert "step_size must be positive" in str(excinfo.value)


def test_walk_forward_splits_uses_test_size_as_default_step():
    df = _make_labeled_frame(12)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    folds = walk_forward_splits(
        dataset,
        initial_train_size=4,
        test_size=3,
        step_size=None,
        max_folds=2,
    )

    assert len(folds) == 2
    assert folds[0].train_end == 4
    assert folds[0].test_start == 4
    assert folds[1].train_end == 7
    assert folds[1].test_start == 7


def test_walk_forward_splits_raises_when_no_folds_generated():
    df = _make_labeled_frame(5)
    dataset = build_training_dataset_from_frame(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=5, test_size=2)
    assert "No walk-forward folds generated" in str(excinfo.value)