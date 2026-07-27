from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

import numpy as np
import pandas as pd

from .labels import HorizonSpec, compute_log_return

FlowColumn = Literal["ofi_l1", "ofi_lN", "weighted_ofi_lN"]
ReturnType = Literal["log_return", "price_change"]


@dataclass(frozen=True)
class KyleLambdaResult:
    flow_column: str
    horizon_events: int
    return_type: str
    n_obs: int
    lambda_: float
    intercept: float
    r2: float


def _future_mid_price(df: pd.DataFrame, horizon_events: int) -> pd.Series:
    if "mid_price" not in df.columns:
        raise ValueError("DataFrame must contain 'mid_price'")
    return df["mid_price"].shift(-horizon_events)


def compute_price_change(
    df: pd.DataFrame,
    horizon_events: int,
) -> pd.Series:
    current = df["mid_price"]
    future = _future_mid_price(df, horizon_events)
    return future - current


def _ols_slope_with_intercept(x: np.ndarray, y: np.ndarray) -> tuple[float, float, float]:
    if x.ndim != 1 or y.ndim != 1:
        raise ValueError("x and y must be 1D arrays")

    if len(x) != len(y):
        raise ValueError("x and y must have the same length")

    if len(x) < 2:
        raise ValueError("Need at least 2 observations")

    x_mean = x.mean()
    y_mean = y.mean()

    x_centered = x - x_mean
    y_centered = y - y_mean

    ss_xx = np.dot(x_centered, x_centered)
    if ss_xx == 0.0:
        raise ValueError("Flow column has zero variance")

    slope = np.dot(x_centered, y_centered) / ss_xx
    intercept = y_mean - slope * x_mean

    y_hat = intercept + slope * x
    ss_res = np.dot(y - y_hat, y - y_hat)
    ss_tot = np.dot(y_centered, y_centered)

    r2 = 1.0 - ss_res / ss_tot if ss_tot > 0.0 else 0.0
    return slope, intercept, r2


def estimate_kyle_lambda(
    df: pd.DataFrame,
    *,
    flow_column: FlowColumn = "ofi_l1",
    horizon_events: int = 50,
    return_type: ReturnType = "price_change",
) -> KyleLambdaResult:
    if flow_column not in df.columns:
        raise ValueError(f"DataFrame must contain flow column: {flow_column}")

    if return_type == "price_change":
        y = compute_price_change(df, horizon_events)
    elif return_type == "log_return":
        y = compute_log_return(df, HorizonSpec(horizon_events=horizon_events))
    else:
        raise ValueError(f"Unsupported return_type: {return_type!r}")

    x = df[flow_column]

    valid = pd.DataFrame({"x": x, "y": y}).dropna()
    if valid.empty:
        raise ValueError("No valid observations after dropping NaNs")

    slope, intercept, r2 = _ols_slope_with_intercept(
        valid["x"].to_numpy(dtype=float),
        valid["y"].to_numpy(dtype=float),
    )

    return KyleLambdaResult(
        flow_column=flow_column,
        horizon_events=horizon_events,
        return_type=return_type,
        n_obs=len(valid),
        lambda_=float(slope),
        intercept=float(intercept),
        r2=float(r2),
    )


def estimate_kyle_lambda_by_window(
    df: pd.DataFrame,
    *,
    flow_column: FlowColumn = "ofi_l1",
    horizon_events: int = 50,
    return_type: ReturnType = "price_change",
    window_size: int = 1000,
    step_size: int | None = None,
) -> pd.DataFrame:
    if step_size is None:
        step_size = window_size

    results: list[dict[str, float | int | str]] = []

    for start in range(0, len(df), step_size):
        end = start + window_size
        window = df.iloc[start:end]
        if len(window) < max(window_size // 2, 2):
            continue

        try:
            est = estimate_kyle_lambda(
                window,
                flow_column=flow_column,
                horizon_events=horizon_events,
                return_type=return_type,
            )
        except ValueError:
            continue

        row: dict[str, float | int | str] = {
            "start_index": int(start),
            "end_index": int(min(end, len(df))),
            "flow_column": est.flow_column,
            "horizon_events": est.horizon_events,
            "return_type": est.return_type,
            "n_obs": est.n_obs,
            "kyle_lambda": est.lambda_,
            "intercept": est.intercept,
            "r2": est.r2,
        }

        if "replay_event_index" in window.columns:
            row["start_replay_event_index"] = int(window["replay_event_index"].iloc[0])
            row["end_replay_event_index"] = int(window["replay_event_index"].iloc[-1])

        if "replay_timestamp_ns" in window.columns:
            row["start_replay_timestamp_ns"] = int(window["replay_timestamp_ns"].iloc[0])
            row["end_replay_timestamp_ns"] = int(window["replay_timestamp_ns"].iloc[-1])

        results.append(row)

    return pd.DataFrame(results)
