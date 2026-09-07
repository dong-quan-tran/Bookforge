from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

import numpy as np
import pandas as pd

LabelType = Literal["regression", "classification"]


@dataclass(frozen=True)
class HorizonSpec:
    """Short-horizon label specification.

    Parameters
    ----------
    horizon_events : int
        Number of events ahead to use when computing the future mid-price.
    """

    horizon_events: int = 50


@dataclass(frozen=True)
class ClassificationThresholds:
    """Thresholds for up/down/flat classification.

    Parameters
    ----------
    up : float
        Minimum log-return to label an observation as up (+1).
    down : float
        Maximum negative log-return to label an observation as down (-1).
        This should be a positive number; the negative is applied internally.
    """

    up: float = 0.0
    down: float = 0.0


def _validate_horizon_events(horizon_events: int) -> None:
    if horizon_events < 1:
        raise ValueError("horizon_events must be positive")


def _future_mid_price(
    df: pd.DataFrame,
    horizon_events: int,
) -> pd.Series:
    if "mid_price" not in df.columns:
        raise ValueError("DataFrame must contain 'mid_price' column for label generation")

    _validate_horizon_events(horizon_events)

    return df["mid_price"].shift(-horizon_events)


def compute_log_return(
    df: pd.DataFrame,
    horizon: HorizonSpec,
) -> pd.Series:
    """Compute short-horizon log return of mid-price.

    Trailing rows where the future price is unavailable are `NaN`.
    """
    future_mid_price = _future_mid_price(df, horizon.horizon_events)
    return np.log(future_mid_price / df["mid_price"])


def classify_return(
    log_ret: pd.Series,
    thresholds: ClassificationThresholds,
) -> pd.Series:
    """Classify log returns into up, down, and flat labels.

    Returns integer labels: +1 for up, -1 for down, and 0 for flat.
    """
    if thresholds.up < 0.0:
        raise ValueError("up threshold must be non-negative")
    if thresholds.down < 0.0:
        raise ValueError("down threshold must be non-negative")

    labels = pd.Series(np.zeros(len(log_ret), dtype=np.int8), index=log_ret.index)

    labels[log_ret >= thresholds.up] = 1
    labels[log_ret <= -thresholds.down] = -1
    labels[log_ret.isna()] = np.nan

    return labels


def make_labels(
    df: pd.DataFrame,
    *,
    horizon_events: int = 50,
    label_type: LabelType = "regression",
    up_threshold: float = 0.0,
    down_threshold: float = 0.0,
) -> pd.Series:
    """Generate short-horizon labels from a feature DataFrame.

    The future mid-price is offset by `horizon_events`; trailing observations
    without a future price receive `NaN` labels.
    """
    horizon = HorizonSpec(horizon_events=horizon_events)
    log_ret = compute_log_return(df, horizon)

    if label_type == "regression":
        return log_ret

    if label_type == "classification":
        thresholds = ClassificationThresholds(
            up=up_threshold,
            down=down_threshold,
        )
        return classify_return(log_ret, thresholds)

    raise ValueError(f"Unsupported label_type: {label_type!r}")
