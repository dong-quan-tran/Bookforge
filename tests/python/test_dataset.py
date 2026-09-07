import numpy as np
import pandas as pd
import pytest
from bookforge_py.dataset import (
    TrainingDataset,
    build_training_dataset_from_frame,
    chronological_split,
    select_feature_columns,
    walk_forward_splits,
)


def _make_base_frame(n: int = 100) -> pd.DataFrame:
    replay_event_index = np.arange(n, dtype=np.int64)
    replay_timestamp_ns = np.arange(n, dtype=np.int64) * 1_000_000
    best_bid = 100.0 + replay_event_index * 0.01
    best_ask = best_bid + 1.0
    spread = best_ask - best_bid
    mid_price = (best_bid + best_ask) / 2.0

    return pd.DataFrame(
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


def _make_labeled_frame(n: int = 100) -> pd.DataFrame:
    df = _make_base_frame(n)
    df["target"] = np.where(df["replay_event_index"] % 2 == 0, -1, 1)
    return df


def _build_classification_dataset(n: int = 100) -> TrainingDataset:
    return build_training_dataset_from_frame(
        _make_labeled_frame(n),
        label_type="classification",
        horizon_events=1,
        up_threshold=0.0,
        down_threshold=0.0,
    )


def test_build_training_dataset_from_frame_classification_normalizes_labels():
    dataset = _build_classification_dataset(10)

    unique_labels = sorted(dataset.y.unique().tolist())

    assert all(isinstance(value, (int, np.integer)) for value in unique_labels)

    if len(unique_labels) > 1:
        assert unique_labels[0] == 0
        assert unique_labels[-1] == len(unique_labels) - 1


def test_build_training_dataset_from_frame_shapes_consistent():
    dataset = _build_classification_dataset(50)

    assert len(dataset.X) == len(dataset.y) == len(dataset.metadata) == len(dataset.full_frame)
    assert "replay_event_index" in dataset.metadata.columns
    assert "replay_timestamp_ns" in dataset.metadata.columns
    assert "target" in dataset.full_frame.columns


def test_chronological_split_preserves_order_and_fraction():
    dataset = _build_classification_dataset(50)

    split = chronological_split(dataset, train_fraction=0.6)
    n_total = len(dataset.X)

    assert len(split.X_train) + len(split.X_test) == n_total
    assert split.split_index == int(n_total * 0.6)
    assert split.purge_events == 0
    assert (
        split.meta_train["replay_event_index"].iloc[-1]
        < split.meta_test["replay_event_index"].iloc[0]
    )


def test_chronological_split_purges_boundary_observations():
    dataset = _build_classification_dataset(50)
    purge_events = 5

    split = chronological_split(
        dataset,
        train_fraction=0.6,
        purge_events=purge_events,
    )

    expected_split_index = int(len(dataset.X) * 0.6)

    assert split.split_index == expected_split_index
    assert split.purge_events == purge_events
    assert len(split.X_train) == expected_split_index - purge_events
    assert split.meta_train["replay_event_index"].iloc[-1] == (
        split.meta_test["replay_event_index"].iloc[0] - purge_events - 1
    )
    assert (
        split.meta_test["replay_event_index"].iloc[0]
        - split.meta_train["replay_event_index"].iloc[-1]
        > split.purge_events
    )


def test_chronological_split_rejects_purge_that_removes_training_rows():
    dataset = _build_classification_dataset(10)

    with pytest.raises(ValueError) as excinfo:
        chronological_split(
            dataset,
            train_fraction=0.5,
            purge_events=5,
        )

    assert "purge_events leaves no training observations" in str(excinfo.value)


def test_chronological_split_rejects_negative_purge():
    dataset = _build_classification_dataset(10)

    with pytest.raises(ValueError) as excinfo:
        chronological_split(
            dataset,
            train_fraction=0.8,
            purge_events=-1,
        )

    assert "purge_events must be non-negative" in str(excinfo.value)


def test_walk_forward_splits_generates_expected_folds():
    dataset = _build_classification_dataset(120)

    folds = walk_forward_splits(
        dataset,
        initial_train_size=60,
        test_size=20,
        step_size=20,
        max_folds=2,
    )

    assert len(folds) == 2

    f0 = folds[0]
    assert f0.train_start == 0
    assert f0.train_end == 60
    assert f0.effective_train_end == 60
    assert f0.test_start == 60
    assert f0.test_end == 80
    assert f0.purge_events == 0
    assert len(f0.X_train) == 60
    assert len(f0.X_test) == 20

    f1 = folds[1]
    assert f1.train_end == 80
    assert f1.effective_train_end == 80
    assert f1.test_start == 80
    assert len(f1.X_train) == 80
    assert len(f1.X_test) == 20

    for fold in folds:
        assert fold.meta_train["replay_event_index"].is_monotonic_increasing
        assert fold.meta_test["replay_event_index"].is_monotonic_increasing


def test_walk_forward_splits_purge_boundary_observations():
    dataset = _build_classification_dataset(120)

    folds = walk_forward_splits(
        dataset,
        initial_train_size=60,
        test_size=20,
        step_size=20,
        max_folds=2,
        purge_events=5,
    )

    assert len(folds) == 2

    f0 = folds[0]
    assert f0.train_end == 60
    assert f0.effective_train_end == 55
    assert f0.test_start == 60
    assert f0.purge_events == 5
    assert len(f0.X_train) == 55
    assert (
        f0.meta_test["replay_event_index"].iloc[0] - f0.meta_train["replay_event_index"].iloc[-1]
        > f0.purge_events
    )

    f1 = folds[1]
    assert f1.train_end == 80
    assert f1.effective_train_end == 75
    assert f1.test_start == 80
    assert len(f1.X_train) == 75
    assert (
        f1.meta_test["replay_event_index"].iloc[0] - f1.meta_train["replay_event_index"].iloc[-1]
        > f1.purge_events
    )


def test_walk_forward_splits_rejects_purge_that_removes_training_rows():
    dataset = _build_classification_dataset(20)

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(
            dataset,
            initial_train_size=5,
            test_size=5,
            purge_events=5,
        )

    assert "purge_events leaves no training observations" in str(excinfo.value)


def test_select_feature_columns_excludes_default_non_features():
    df = _make_labeled_frame(10)
    columns = select_feature_columns(df)

    assert "symbol" not in columns
    assert "replay_event_index" not in columns
    assert "replay_timestamp_ns" not in columns
    assert "best_bid" in columns
    assert "best_ask" in columns


def test_select_feature_columns_respects_custom_exclusions():
    df = _make_labeled_frame(10)
    columns = select_feature_columns(df, exclude_columns={"symbol", "spread"})

    assert "symbol" not in columns
    assert "spread" not in columns
    assert "best_bid" in columns


@pytest.mark.parametrize(
    ("train_fraction", "expected_message"),
    [
        (0.0, "train_fraction must be between 0 and 1"),
        (1.0, "train_fraction must be between 0 and 1"),
    ],
)
def test_chronological_split_rejects_invalid_fraction(
    train_fraction: float,
    expected_message: str,
):
    dataset = _build_classification_dataset(10)

    with pytest.raises(ValueError) as excinfo:
        chronological_split(dataset, train_fraction=train_fraction)

    assert expected_message in str(excinfo.value)


def test_chronological_split_rejects_too_small_dataset():
    dataset = _build_classification_dataset(1)

    with pytest.raises(ValueError) as excinfo:
        chronological_split(dataset, train_fraction=0.8)

    assert "Need at least 2 rows to split dataset" in str(excinfo.value)


@pytest.mark.parametrize(
    ("initial_train_size", "test_size", "step_size", "expected_message"),
    [
        (0, 2, None, "initial_train_size must be positive"),
        (5, 0, None, "test_size must be positive"),
        (5, 2, 0, "step_size must be positive"),
    ],
)
def test_walk_forward_splits_rejects_invalid_window_parameters(
    initial_train_size: int,
    test_size: int,
    step_size: int | None,
    expected_message: str,
):
    dataset = _build_classification_dataset(10)

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(
            dataset,
            initial_train_size=initial_train_size,
            test_size=test_size,
            step_size=step_size,
        )

    assert expected_message in str(excinfo.value)


def test_walk_forward_splits_rejects_small_dataset():
    dataset = _build_classification_dataset(2)

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=1, test_size=1)

    assert "Need at least 3 rows for walk-forward validation" in str(excinfo.value)


def test_walk_forward_splits_uses_test_size_as_default_step():
    dataset = _build_classification_dataset(12)

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
    dataset = _build_classification_dataset(5)

    with pytest.raises(ValueError) as excinfo:
        walk_forward_splits(dataset, initial_train_size=5, test_size=2)

    assert "No walk-forward folds generated" in str(excinfo.value)
