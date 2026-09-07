import numpy as np
import pandas as pd
import pytest
from bookforge_py.labels import (
    ClassificationThresholds,
    HorizonSpec,
    classify_return,
    compute_log_return,
    make_labels,
)


def _price_frame(prices: list[float]) -> pd.DataFrame:
    return pd.DataFrame(
        {
            "mid_price": prices,
        }
    )


def test_compute_log_return_uses_future_event_horizon():
    df = _price_frame([100.0, 110.0, 121.0, 133.1])

    returns = compute_log_return(df, HorizonSpec(horizon_events=1))

    assert returns.iloc[0] == pytest.approx(np.log(1.1))
    assert returns.iloc[1] == pytest.approx(np.log(1.1))
    assert returns.iloc[2] == pytest.approx(np.log(1.1))
    assert np.isnan(returns.iloc[3])


def test_compute_log_return_supports_multi_event_horizon():
    df = _price_frame([100.0, 101.0, 121.0, 122.0])

    returns = compute_log_return(df, HorizonSpec(horizon_events=2))

    assert returns.iloc[0] == pytest.approx(np.log(121.0 / 100.0))
    assert returns.iloc[1] == pytest.approx(np.log(122.0 / 101.0))
    assert np.isnan(returns.iloc[2])
    assert np.isnan(returns.iloc[3])


def test_compute_log_return_requires_mid_price():
    with pytest.raises(ValueError, match="mid_price"):
        compute_log_return(pd.DataFrame({"price": [100.0, 101.0]}), HorizonSpec())


def test_classify_return_applies_up_down_and_flat_thresholds():
    returns = pd.Series(
        [
            -0.03,
            -0.01,
            0.0,
            0.01,
            0.03,
            np.nan,
        ]
    )

    labels = classify_return(
        returns,
        ClassificationThresholds(up=0.02, down=0.02),
    )

    assert labels.iloc[0] == -1
    assert labels.iloc[1] == 0
    assert labels.iloc[2] == 0
    assert labels.iloc[3] == 0
    assert labels.iloc[4] == 1
    assert np.isnan(labels.iloc[5])


def test_make_labels_generates_regression_targets():
    df = _price_frame([100.0, 110.0, 121.0])

    labels = make_labels(
        df,
        label_type="regression",
        horizon_events=1,
    )

    assert labels.iloc[0] == pytest.approx(np.log(1.1))
    assert labels.iloc[1] == pytest.approx(np.log(1.1))
    assert np.isnan(labels.iloc[2])


def test_make_labels_generates_classification_targets():
    df = _price_frame([100.0, 103.0, 100.0, 100.0])

    labels = make_labels(
        df,
        label_type="classification",
        horizon_events=1,
        up_threshold=0.01,
        down_threshold=0.01,
    )

    assert labels.iloc[0] == 1
    assert labels.iloc[1] == -1
    assert labels.iloc[2] == 0
    assert np.isnan(labels.iloc[3])


def test_make_labels_rejects_unknown_label_type():
    df = _price_frame([100.0, 101.0])

    with pytest.raises(ValueError, match="Unsupported label_type"):
        make_labels(
            df,
            label_type="unsupported",  # type: ignore[arg-type]
        )
