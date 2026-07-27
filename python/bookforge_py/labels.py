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
        Minimum log-return to label an observation as 'up' (+1).
    down : float
        Maximum log-return to label an observation as 'down' (-1).
        This should be a positive number; the negative is applied internally.
    """

    up: float = 0.0
    down: float = 0.0


def _future_mid_price(
    df: pd.DataFrame,
    horizon_events: int,
) -> pd.Series:
    if "mid_price" not in df.columns:
        raise ValueError("DataFrame must contain 'mid_price' column for label generation")

    # Shift mid_price by horizon_events to align future value with current row
    return df["mid_price"].shift(-horizon_events)


def compute_log_return(
    df: pd.DataFrame,
    horizon: HorizonSpec,
) -> pd.Series:
    """Compute short-horizon log-return of mid_price.

    Returns
    -------
    pandas.Series
        Log-return over the given event horizon. Trailing rows where the
        future price is not available will be NaN.
    """
    m0 = df["mid_price"]
    m_future = _future_mid_price(df, horizon.horizon_events)

    return np.log(m_future / m0)


def classify_return(
    log_ret: pd.Series,
    thresholds: ClassificationThresholds,
) -> pd.Series:
    """Classify log-returns into up/down/flat labels.

    Returns
    -------
    pandas.Series
        Integer labels: +1 (up), -1 (down), 0 (flat).
    """
    labels = pd.Series(np.zeros(len(log_ret), dtype=np.int8), index=log_ret.index)

    up_mask = log_ret >= thresholds.up
    down_mask = log_ret <= -thresholds.down

    labels[up_mask] = 1
    labels[down_mask] = -1

    # NaN returns get NaN labels to allow filtering
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

    Parameters
    ----------
    df : pandas.DataFrame
        Feature DataFrame containing at least 'mid_price'.
    horizon_events : int, optional
        Number of events ahead to use when computing the future mid-price.
    label_type : {'regression', 'classification'}, optional
        Type of label to generate.
    up_threshold : float, optional
        Minimum log-return for 'up' (+1) classification labels.
    down_threshold : float, optional
        Minimum absolute log-return for 'down' (-1) classification labels.

    Returns
    -------
    pandas.Series
        Labels aligned with df.index. Trailing rows will be NaN.
    """
    horizon = HorizonSpec(horizon_events=horizon_events)
    log_ret = compute_log_return(df, horizon)

    if label_type == "regression":
        return log_ret
    elif label_type == "classification":
        thresholds = ClassificationThresholds(up=up_threshold, down=down_threshold)
        return classify_return(log_ret, thresholds)
    else:
        raise ValueError(f"Unsupported label_type: {label_type!r}")
