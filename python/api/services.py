from __future__ import annotations

from pathlib import Path
from typing import Any

import pandas as pd
from fastapi import HTTPException

DEFAULT_FEATURES_CSV = Path("output/features.csv")
DEFAULT_SNAPSHOTS_CSV = Path("output/snapshots.csv")


def _resolve_existing_file(path_str: str | None, default_path: Path) -> Path:
    path = Path(path_str) if path_str else default_path
    if not path.exists():
        raise HTTPException(status_code=404, detail=f"File not found: {path}")
    if not path.is_file():
        raise HTTPException(status_code=400, detail=f"Not a file: {path}")
    return path


def read_csv(path_str: str | None, default_path: Path) -> tuple[Path, pd.DataFrame]:
    path = _resolve_existing_file(path_str, default_path)
    try:
        df = pd.read_csv(path)
    except Exception as exc:
        raise HTTPException(status_code=400, detail=f"Failed to read CSV {path}: {exc}") from exc
    if df.empty:
        raise HTTPException(status_code=400, detail=f"CSV is empty: {path}")
    return path, df


def _first_present(df: pd.DataFrame, candidates: list[str]) -> str | None:
    for col in candidates:
        if col in df.columns:
            return col
    return None


def _safe_mean(df: pd.DataFrame, column: str) -> float | None:
    if column not in df.columns:
        return None
    series = pd.to_numeric(df[column], errors="coerce").dropna()
    if series.empty:
        return None
    return float(series.mean())


def _coerce_float(value: Any) -> float | None:
    if pd.isna(value):
        return None
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def _coerce_int(value: Any) -> int | None:
    if pd.isna(value):
        return None
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


def _clean_row_values(row: dict[str, Any]) -> dict[str, Any]:
    cleaned: dict[str, Any] = {}
    for key, value in row.items():
        if pd.isna(value):
            cleaned[str(key)] = None
        else:
            cleaned[str(key)] = value
    return cleaned


def _find_level_key(row: dict[str, Any], candidates: list[str]) -> str | None:
    for key in candidates:
        if key in row:
            return key
    return None


def _extract_side_levels(row: dict[str, Any], side_prefix: str) -> list[dict[str, Any]]:
    levels: list[dict[str, Any]] = []
    level = 1

    while True:
        price_key = _find_level_key(
            row,
            [
                f"{side_prefix}_price_{level}",
                f"{side_prefix}_{level}_price",
                f"{side_prefix}{level}_price",
                f"{side_prefix}_px_{level}",
                f"{side_prefix}_{level}_px",
                f"{side_prefix}{level}_px",
                f"top_{side_prefix}_price_{level}",
            ],
        )
        size_key = _find_level_key(
            row,
            [
                f"{side_prefix}_size_{level}",
                f"{side_prefix}_{level}_size",
                f"{side_prefix}{level}_size",
                f"{side_prefix}_qty_{level}",
                f"{side_prefix}_{level}_qty",
                f"{side_prefix}{level}_qty",
                f"top_{side_prefix}_size_{level}",
                f"top_{side_prefix}_qty_{level}",
            ],
        )

        if price_key is None and size_key is None:
            break

        levels.append(
            {
                "level": level,
                "price": _coerce_float(row.get(price_key)) if price_key else None,
                "size": _coerce_float(row.get(size_key)) if size_key else None,
            }
        )
        level += 1

    return levels


def build_replay_summary(features_csv: str | None) -> dict[str, Any]:
    path, df = read_csv(features_csv, DEFAULT_FEATURES_CSV)

    event_col = _first_present(df, ["replay_event_index", "event_index"])
    ts_col = _first_present(df, ["replay_timestamp_ns", "timestamp_ns", "ts_ns"])
    symbol_col = _first_present(df, ["symbol"])
    imbalance_col = _first_present(
        df,
        ["depth_imbalance", "imbalance", "l1_depth_imbalance"],
    )

    symbol = None
    if symbol_col is not None and not df[symbol_col].dropna().empty:
        symbol = str(df[symbol_col].dropna().iloc[0])

    return {
        "features_csv": str(path),
        "row_count": int(len(df)),
        "column_count": int(len(df.columns)),
        "columns": [str(c) for c in df.columns.tolist()],
        "symbol": symbol,
        "start_event_index": int(df[event_col].iloc[0]) if event_col else None,
        "end_event_index": int(df[event_col].iloc[-1]) if event_col else None,
        "start_timestamp_ns": int(df[ts_col].iloc[0]) if ts_col else None,
        "end_timestamp_ns": int(df[ts_col].iloc[-1]) if ts_col else None,
        "avg_spread": _safe_mean(df, "spread"),
        "avg_mid_price": _safe_mean(df, "mid_price"),
        "avg_depth_imbalance": _safe_mean(df, imbalance_col) if imbalance_col else None,
    }


def build_feature_sample(
    features_csv: str | None,
    limit: int,
    offset: int = 0,
) -> dict[str, Any]:
    path, df = read_csv(features_csv, DEFAULT_FEATURES_CSV)

    if limit < 1 or limit > 5000:
        raise HTTPException(status_code=400, detail="limit must be between 1 and 5000")
    if offset < 0:
        raise HTTPException(status_code=400, detail="offset must be >= 0")

    sliced = df.iloc[offset : offset + limit].reset_index(drop=True)
    rows = [
        {"row_index": int(offset + i), "values": _clean_row_values(row)}
        for i, row in enumerate(sliced.to_dict(orient="records"))
    ]

    return {
        "features_csv": str(path),
        "row_count": int(len(df)),
        "returned_count": int(len(rows)),
        "rows": rows,
    }


def build_snapshot_sample(
    snapshot_csv: str | None,
    limit: int,
    offset: int = 0,
) -> dict[str, Any]:
    path, df = read_csv(snapshot_csv, DEFAULT_SNAPSHOTS_CSV)

    if limit < 1 or limit > 5000:
        raise HTTPException(status_code=400, detail="limit must be between 1 and 5000")
    if offset < 0:
        raise HTTPException(status_code=400, detail="offset must be >= 0")

    sliced = df.iloc[offset : offset + limit].reset_index(drop=True)
    rows = [
        {"row_index": int(offset + i), "values": _clean_row_values(row)}
        for i, row in enumerate(sliced.to_dict(orient="records"))
    ]

    return {
        "snapshot_csv": str(path),
        "row_count": int(len(df)),
        "returned_count": int(len(rows)),
        "rows": rows,
    }


def build_snapshot_inspect(
    snapshot_csv: str | None,
    row_index: int,
) -> dict[str, Any]:
    path, df = read_csv(snapshot_csv, DEFAULT_SNAPSHOTS_CSV)

    if row_index < 0:
        raise HTTPException(status_code=400, detail="row_index must be >= 0")
    if row_index >= len(df):
        raise HTTPException(
            status_code=404,
            detail=f"row_index {row_index} out of range for {path} with {len(df)} rows",
        )

    raw_row = df.iloc[row_index].to_dict()
    row = _clean_row_values(raw_row)

    symbol_col = _first_present(df, ["symbol"])
    event_col = _first_present(df, ["replay_event_index", "event_index"])
    ts_col = _first_present(df, ["replay_timestamp_ns", "timestamp_ns", "ts_ns"])

    bids = _extract_side_levels(row, "bid")
    asks = _extract_side_levels(row, "ask")

    return {
        "snapshot_csv": str(path),
        "row_index": int(row_index),
        "total_rows": int(len(df)),
        "symbol": str(row.get(symbol_col)) if symbol_col and row.get(symbol_col) is not None else None,
        "replay_event_index": _coerce_int(row.get(event_col)) if event_col else None,
        "replay_timestamp_ns": _coerce_int(row.get(ts_col)) if ts_col else None,
        "best_bid": _coerce_float(row.get("best_bid")),
        "best_ask": _coerce_float(row.get("best_ask")),
        "spread": _coerce_float(row.get("spread")),
        "mid_price": _coerce_float(row.get("mid_price")),
        "bids": bids,
        "asks": asks,
        "raw_values": row,
    }


def build_columns_response(features_csv: str | None) -> dict[str, Any]:
    path, df = read_csv(features_csv, DEFAULT_FEATURES_CSV)
    return {
        "features_csv": str(path),
        "columns": [str(c) for c in df.columns.tolist()],
    }