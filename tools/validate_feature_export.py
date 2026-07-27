from __future__ import annotations

import sys
from pathlib import Path

import pandas as pd
from pandas.api.types import is_numeric_dtype

REQUIRED_COLUMNS = [
    "symbol",
    "replay_event_index",
    "replay_timestamp_ns",
    "best_bid",
    "best_ask",
    "spread",
    "mid_price",
    "l1_bid_qty",
    "l1_ask_qty",
    "l1_depth_imbalance",
    "lN_bid_qty_sum",
    "lN_ask_qty_sum",
    "lN_depth_imbalance",
    "ofi_l1",
    "ofi_lN",
    "weighted_ofi_lN",
    "rolling_mean_spread",
    "rolling_mean_l1_total_depth",
    "rolling_mean_lN_total_depth",
    "rolling_mid_return",
    "rolling_realized_mid_vol",
    "rolling_mean_abs_ofi_l1",
    "rolling_mean_abs_ofi_lN",
]

NUMERIC_COLUMNS = [
    "replay_event_index",
    "replay_timestamp_ns",
    "best_bid",
    "best_ask",
    "spread",
    "mid_price",
    "l1_bid_qty",
    "l1_ask_qty",
    "l1_depth_imbalance",
    "lN_bid_qty_sum",
    "lN_ask_qty_sum",
    "lN_depth_imbalance",
    "ofi_l1",
    "ofi_lN",
    "weighted_ofi_lN",
    "rolling_mean_spread",
    "rolling_mean_l1_total_depth",
    "rolling_mean_lN_total_depth",
    "rolling_mid_return",
    "rolling_realized_mid_vol",
    "rolling_mean_abs_ofi_l1",
    "rolling_mean_abs_ofi_lN",
]

IMBALANCE_COLUMNS = [
    "l1_depth_imbalance",
    "lN_depth_imbalance",
]

NONNEGATIVE_COLUMNS = [
    "spread",
    "l1_bid_qty",
    "l1_ask_qty",
    "lN_bid_qty_sum",
    "lN_ask_qty_sum",
    "rolling_mean_spread",
    "rolling_mean_l1_total_depth",
    "rolling_mean_lN_total_depth",
    "rolling_realized_mid_vol",
    "rolling_mean_abs_ofi_l1",
    "rolling_mean_abs_ofi_lN",
]


def fail(message: str) -> None:
    raise SystemExit(f"[feature-export-validation] ERROR: {message}")


def main() -> int:
    if len(sys.argv) != 2:
        fail("usage: python tools/validate_feature_export.py <features.csv>")

    csv_path = Path(sys.argv[1])
    if not csv_path.exists():
        fail(f"file does not exist: {csv_path}")

    df = pd.read_csv(csv_path)

    missing = [c for c in REQUIRED_COLUMNS if c not in df.columns]
    extra = [c for c in df.columns if c not in REQUIRED_COLUMNS]

    if missing:
        fail(f"missing required columns: {missing}")
    if extra:
        fail(f"unexpected extra columns: {extra}")

    if df.empty:
        fail("feature export is empty")

    if not pd.api.types.is_string_dtype(df["symbol"]):
        fail("column 'symbol' must be string-like")

    for col in NUMERIC_COLUMNS:
        if not is_numeric_dtype(df[col]):
            fail(f"column '{col}' must be numeric")

    for col in IMBALANCE_COLUMNS:
        series = df[col].dropna()
        if not ((series >= -1.0) & (series <= 1.0)).all():
            fail(f"column '{col}' contains values outside [-1, 1]")

    for col in NONNEGATIVE_COLUMNS:
        series = df[col].dropna()
        if not (series >= 0.0).all():
            fail(f"column '{col}' contains negative values")

    idx = df["replay_event_index"].dropna()
    if not idx.is_monotonic_increasing:
        fail("replay_event_index must be monotonically increasing")

    ts = df["replay_timestamp_ns"].dropna()
    if not ts.is_monotonic_increasing:
        fail("replay_timestamp_ns must be monotonically increasing")

    if {"best_bid", "best_ask", "spread"}.issubset(df.columns):
        mask = df["best_bid"].notna() & df["best_ask"].notna() & df["spread"].notna()
        if mask.any():
            expected_spread = df.loc[mask, "best_ask"] - df.loc[mask, "best_bid"]
            actual_spread = df.loc[mask, "spread"]
            bad = (expected_spread - actual_spread).abs() > 1e-9
            if bad.any():
                fail("spread does not match best_ask - best_bid")

    if {"best_bid", "best_ask", "mid_price"}.issubset(df.columns):
        mask = df["best_bid"].notna() & df["best_ask"].notna() & df["mid_price"].notna()
        if mask.any():
            expected_mid = (df.loc[mask, "best_bid"] + df.loc[mask, "best_ask"]) / 2.0
            actual_mid = df.loc[mask, "mid_price"]
            bad = (expected_mid - actual_mid).abs() > 1e-9
            if bad.any():
                fail("mid_price does not match (best_bid + best_ask) / 2")

    print("[feature-export-validation] OK")
    print(f"rows={len(df)}")
    print(f"columns={len(df.columns)}")
    print(f"first_event_index={df['replay_event_index'].iloc[0]}")
    print(f"last_event_index={df['replay_event_index'].iloc[-1]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
