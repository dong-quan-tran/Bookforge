from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Literal

import pandas as pd

from .labels import make_labels
from .loaders import DEFAULT_METADATA_COLUMNS, load_feature_csv

LabelType = Literal["regression", "classification"]


@dataclass(frozen=True)
class TrainingDataset:
    X: pd.DataFrame
    y: pd.Series
    metadata: pd.DataFrame
    full_frame: pd.DataFrame
    label_column: str


@dataclass(frozen=True)
class ChronologicalSplit:
    X_train: pd.DataFrame
    y_train: pd.Series
    meta_train: pd.DataFrame
    X_test: pd.DataFrame
    y_test: pd.Series
    meta_test: pd.DataFrame


@dataclass(frozen=True)
class WalkForwardFold:
    fold_index: int
    train_start: int
    train_end: int
    test_start: int
    test_end: int
    X_train: pd.DataFrame
    y_train: pd.Series
    meta_train: pd.DataFrame
    X_test: pd.DataFrame
    y_test: pd.Series
    meta_test: pd.DataFrame


DEFAULT_EXCLUDED_FEATURE_COLUMNS = {
    "symbol",
    "replay_event_index",
    "replay_timestamp_ns",
}


def select_feature_columns(
    df: pd.DataFrame,
    *,
    exclude_columns: Iterable[str] = DEFAULT_EXCLUDED_FEATURE_COLUMNS,
) -> list[str]:
    exclude = set(exclude_columns)
    return [col for col in df.columns if col not in exclude]


def _normalize_classification_target(y: pd.Series) -> pd.Series:
    y = y.astype("int64")
    unique_labels = sorted(y.dropna().unique().tolist())
    label_mapping = {label: idx for idx, label in enumerate(unique_labels)}
    return y.map(label_mapping).astype("int8")


def build_training_dataset(
    path: str | Path,
    *,
    label_type: LabelType = "regression",
    horizon_events: int = 50,
    up_threshold: float = 0.0,
    down_threshold: float = 0.0,
    metadata_columns: Iterable[str] = DEFAULT_METADATA_COLUMNS,
    exclude_feature_columns: Iterable[str] = DEFAULT_EXCLUDED_FEATURE_COLUMNS,
    label_column: str = "target",
) -> TrainingDataset:
    df = load_feature_csv(path).copy()

    labels = make_labels(
        df,
        horizon_events=horizon_events,
        label_type=label_type,
        up_threshold=up_threshold,
        down_threshold=down_threshold,
    )

    df[label_column] = labels

    metadata_columns = list(metadata_columns)
    feature_cols = select_feature_columns(
        df.drop(columns=[label_column]),
        exclude_columns=exclude_feature_columns,
    )

    required_columns = metadata_columns + feature_cols + [label_column]
    model_df = df[required_columns].dropna().reset_index(drop=True)

    metadata = model_df[metadata_columns].copy()
    X = model_df[feature_cols].copy()
    y = model_df[label_column].copy()

    if label_type == "classification":
        y = _normalize_classification_target(y)

    return TrainingDataset(
        X=X,
        y=y,
        metadata=metadata,
        full_frame=model_df,
        label_column=label_column,
    )


def chronological_split(
    dataset: TrainingDataset,
    *,
    train_fraction: float = 0.8,
) -> ChronologicalSplit:
    if not 0.0 < train_fraction < 1.0:
        raise ValueError("train_fraction must be between 0 and 1")

    n = len(dataset.X)
    if n < 2:
        raise ValueError("Need at least 2 rows to split dataset")

    split_idx = int(n * train_fraction)
    split_idx = max(1, min(split_idx, n - 1))

    return ChronologicalSplit(
        X_train=dataset.X.iloc[:split_idx].reset_index(drop=True),
        y_train=dataset.y.iloc[:split_idx].reset_index(drop=True),
        meta_train=dataset.metadata.iloc[:split_idx].reset_index(drop=True),
        X_test=dataset.X.iloc[split_idx:].reset_index(drop=True),
        y_test=dataset.y.iloc[split_idx:].reset_index(drop=True),
        meta_test=dataset.metadata.iloc[split_idx:].reset_index(drop=True),
    )


def walk_forward_splits(
    dataset: TrainingDataset,
    *,
    initial_train_size: int,
    test_size: int,
    step_size: int | None = None,
    max_folds: int | None = None,
) -> list[WalkForwardFold]:
    n = len(dataset.X)
    if n < 3:
        raise ValueError("Need at least 3 rows for walk-forward validation")
    if initial_train_size <= 0:
        raise ValueError("initial_train_size must be positive")
    if test_size <= 0:
        raise ValueError("test_size must be positive")

    step = test_size if step_size is None else step_size
    if step <= 0:
        raise ValueError("step_size must be positive")

    folds: list[WalkForwardFold] = []
    train_end = initial_train_size
    fold_index = 0

    while train_end < n:
        test_start = train_end
        test_end = min(test_start + test_size, n)

        if test_start >= test_end:
            break

        folds.append(
            WalkForwardFold(
                fold_index=fold_index,
                train_start=0,
                train_end=train_end,
                test_start=test_start,
                test_end=test_end,
                X_train=dataset.X.iloc[:train_end].reset_index(drop=True),
                y_train=dataset.y.iloc[:train_end].reset_index(drop=True),
                meta_train=dataset.metadata.iloc[:train_end].reset_index(drop=True),
                X_test=dataset.X.iloc[test_start:test_end].reset_index(drop=True),
                y_test=dataset.y.iloc[test_start:test_end].reset_index(drop=True),
                meta_test=dataset.metadata.iloc[test_start:test_end].reset_index(drop=True),
            )
        )

        fold_index += 1
        if max_folds is not None and fold_index >= max_folds:
            break

        train_end += step

    if not folds:
        raise ValueError("No walk-forward folds generated; adjust window sizes")

    return folds


def build_training_dataset_from_frame(
    df: pd.DataFrame,
    *,
    label_type: LabelType = "regression",
    horizon_events: int = 50,
    up_threshold: float = 0.0,
    down_threshold: float = 0.0,
    metadata_columns: Iterable[str] = DEFAULT_METADATA_COLUMNS,
    exclude_feature_columns: Iterable[str] = DEFAULT_EXCLUDED_FEATURE_COLUMNS,
    label_column: str = "target",
) -> TrainingDataset:
    working = df.copy()

    labels = make_labels(
        working,
        horizon_events=horizon_events,
        label_type=label_type,
        up_threshold=up_threshold,
        down_threshold=down_threshold,
    )

    working[label_column] = labels

    metadata_columns = list(metadata_columns)
    feature_cols = select_feature_columns(
        working.drop(columns=[label_column]),
        exclude_columns=exclude_feature_columns,
    )

    required_columns = metadata_columns + feature_cols + [label_column]
    model_df = working[required_columns].dropna().reset_index(drop=True)

    metadata = model_df[metadata_columns].copy()
    X = model_df[feature_cols].copy()
    y = model_df[label_column].copy()

    if label_type == "classification":
        y = _normalize_classification_target(y)

    return TrainingDataset(
        X=X,
        y=y,
        metadata=metadata,
        full_frame=model_df,
        label_column=label_column,
    )
