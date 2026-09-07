import numpy as np
import pandas as pd
import pytest
from bookforge_py.impact import (
    compute_price_change,
    estimate_kyle_lambda,
    estimate_kyle_lambda_by_window,
)


def _impact_frame() -> pd.DataFrame:
    flow = np.array([-2.0, -1.0, 0.0, 1.0, 2.0, 3.0])
    one_step_price_change = 0.5 * flow
    mid_price = 100.0 + np.concatenate(
        [
            [0.0],
            np.cumsum(one_step_price_change),
        ]
    )

    return pd.DataFrame(
        {
            "mid_price": mid_price,
            "ofi_l1": flow.tolist() + [0.0],
            "replay_event_index": list(range(len(mid_price))),
            "replay_timestamp_ns": [index * 1_000_000 for index in range(len(mid_price))],
        }
    )


def test_compute_price_change_uses_future_event_horizon():
    df = pd.DataFrame({"mid_price": [100.0, 101.5, 103.0, 104.5]})

    price_change = compute_price_change(df, horizon_events=2)

    assert price_change.iloc[0] == pytest.approx(3.0)
    assert price_change.iloc[1] == pytest.approx(3.0)
    assert np.isnan(price_change.iloc[2])
    assert np.isnan(price_change.iloc[3])


def test_estimate_kyle_lambda_recovers_linear_slope():
    df = _impact_frame()

    result = estimate_kyle_lambda(
        df,
        flow_column="ofi_l1",
        horizon_events=1,
        return_type="price_change",
    )

    assert result.flow_column == "ofi_l1"
    assert result.horizon_events == 1
    assert result.return_type == "price_change"
    assert result.n_obs == 6
    assert result.lambda_ == pytest.approx(0.5)
    assert result.intercept == pytest.approx(0.0)
    assert result.r2 == pytest.approx(1.0)


def test_estimate_kyle_lambda_requires_flow_column():
    df = pd.DataFrame({"mid_price": [100.0, 101.0, 102.0]})

    with pytest.raises(ValueError, match="flow column"):
        estimate_kyle_lambda(df, flow_column="ofi_l1")


def test_estimate_kyle_lambda_rejects_zero_variance_flow():
    df = pd.DataFrame(
        {
            "mid_price": [100.0, 101.0, 102.0, 103.0],
            "ofi_l1": [1.0, 1.0, 1.0, 1.0],
        }
    )

    with pytest.raises(ValueError, match="zero variance"):
        estimate_kyle_lambda(df, horizon_events=1)


def test_estimate_kyle_lambda_by_window_returns_window_metadata():
    df = _impact_frame()

    results = estimate_kyle_lambda_by_window(
        df,
        flow_column="ofi_l1",
        horizon_events=1,
        return_type="price_change",
        window_size=4,
        step_size=2,
    )

    assert not results.empty
    assert {
        "start_index",
        "end_index",
        "flow_column",
        "horizon_events",
        "return_type",
        "n_obs",
        "kyle_lambda",
        "intercept",
        "r2",
        "start_replay_event_index",
        "end_replay_event_index",
        "start_replay_timestamp_ns",
        "end_replay_timestamp_ns",
    }.issubset(results.columns)

    assert (results["n_obs"] >= 2).all()
    assert (results["flow_column"] == "ofi_l1").all()
