import bookforge_py as bf
import pytest


def _native_bindings_available() -> bool:
    try:
        _ = bf.BookSnapshot
    except ImportError:
        return False
    return True


pytestmark = pytest.mark.skipif(
    not _native_bindings_available(),
    reason="Bookforge native bindings have not been built",
)


def test_native_snapshot_types_are_exported_through_package():
    depth = bf.DepthLevelSnapshot()
    depth.price = 100.25
    depth.quantity = 42

    snapshot = bf.BookSnapshot()
    snapshot.symbol = "BTCUSDT.P"
    snapshot.replay_event_index = 7
    snapshot.replay_timestamp_ns = 1_000_000
    snapshot.bids = [depth]

    assert snapshot.symbol == "BTCUSDT.P"
    assert snapshot.replay_event_index == 7
    assert snapshot.replay_timestamp_ns == 1_000_000
    assert len(snapshot.bids) == 1
    assert snapshot.bids[0].price == pytest.approx(100.25)
    assert snapshot.bids[0].quantity == 42


def test_native_feature_row_types_are_exported_through_package():
    feature = bf.FeatureRow()
    feature.symbol = "BTCUSDT.P"
    feature.replay_event_index = 9
    feature.mid_price = 100.5
    feature.spread = 0.5
    feature.l1_depth_imbalance = 0.25

    assert feature.symbol == "BTCUSDT.P"
    assert feature.replay_event_index == 9
    assert feature.mid_price == pytest.approx(100.5)
    assert feature.spread == pytest.approx(0.5)
    assert feature.l1_depth_imbalance == pytest.approx(0.25)
