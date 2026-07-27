from __future__ import annotations

from collections.abc import Iterable
from pathlib import Path

import numpy as np
import pandas as pd

REQUIRED_FEATURE_COLUMNS = [
    "replay_event_index",
    "replay_timestamp_ns",
    "best_bid",
    "best_ask",
    "spread",
    "mid_price",
]

DEFAULT_METADATA_COLUMNS = [
    "replay_event_index",
    "replay_timestamp_ns",
]

NON_FEATURE_COLUMNS = set(DEFAULT_METADATA_COLUMNS)


def _ensure_columns(df: pd.DataFrame, required: Iterable[str]) -> None:
    missing = [col for col in required if col not in df.columns]
    if missing:
        raise ValueError(f"Missing required columns: {missing}")


def _coerce_numeric_columns(df: pd.DataFrame, columns: Iterable[str]) -> pd.DataFrame:
    out = df.copy()
    for col in columns:
        if col in out.columns:
            out[col] = pd.to_numeric(out[col], errors="raise")
    return out


def validate_feature_frame(df: pd.DataFrame) -> None:
    _ensure_columns(df, REQUIRED_FEATURE_COLUMNS)

    if not df["replay_event_index"].is_monotonic_increasing:
        raise ValueError("replay_event_index must be monotonic increasing")

    if not df["replay_timestamp_ns"].is_monotonic_increasing:
        raise ValueError("replay_timestamp_ns must be monotonic increasing")

    mask = df["spread"].notna() & df["best_ask"].notna() & df["best_bid"].notna()
    expected_spread = df.loc[mask, "best_ask"] - df.loc[mask, "best_bid"]

    if not np.isclose(
        df.loc[mask, "spread"],
        expected_spread,
        rtol=1e-9,
        atol=1e-9,
    ).all():
        raise ValueError("spread must approximately equal best_ask - best_bid")

    mid_mask = df["mid_price"].notna() & df["best_bid"].notna() & df["best_ask"].notna()
    expected_mid = (df.loc[mid_mask, "best_bid"] + df.loc[mid_mask, "best_ask"]) / 2.0

    if not np.isclose(
        df.loc[mid_mask, "mid_price"],
        expected_mid,
        rtol=1e-9,
        atol=1e-9,
    ).all():
        raise ValueError("mid_price must approximately equal (best_bid + best_ask) / 2")

    non_negative_columns = [
        "spread",
        "l1_bid_qty",
        "l1_ask_qty",
        "rolling_mean_spread",
        "rolling_mean_l1_total_depth",
        "rolling_mean_lN_total_depth",
        "rolling_realized_mid_vol",
        "rolling_mean_abs_ofi_l1",
        "rolling_mean_abs_ofi_lN",
    ]

    for col in non_negative_columns:
        if col in df.columns and (df[col] < 0).any():
            raise ValueError(f"{col} must be non-negative")

    bounded_columns = [
        "l1_depth_imbalance",
        "lN_depth_imbalance",
    ]

    for col in bounded_columns:
        if col in df.columns and ((df[col] < -1) | (df[col] > 1)).any():
            raise ValueError(f"{col} must stay within [-1, 1]")


def load_feature_csv(path: str | Path) -> pd.DataFrame:
    csv_path = Path(path)
    df = pd.read_csv(csv_path)

    _ensure_columns(df, REQUIRED_FEATURE_COLUMNS)

    numeric_columns = [col for col in df.columns if col not in ["symbol"]]
    df = _coerce_numeric_columns(df, numeric_columns)

    validate_feature_frame(df)
    return df


def split_feature_columns(
    df: pd.DataFrame,
    metadata_columns: Iterable[str] = DEFAULT_METADATA_COLUMNS,
) -> tuple[pd.DataFrame, pd.DataFrame]:
    metadata_columns = list(metadata_columns)
    _ensure_columns(df, metadata_columns)

    meta = df[metadata_columns].copy()
    features = df.drop(columns=metadata_columns).copy()
    return meta, features


def feature_column_names(
    df: pd.DataFrame,
    metadata_columns: Iterable[str] = DEFAULT_METADATA_COLUMNS,
) -> list[str]:
    metadata_columns = set(metadata_columns)
    return [col for col in df.columns if col not in metadata_columns]
