#include "snapshot/SnapshotComparator.hpp"

#include <sstream>

namespace bookforge {
namespace {

bool OptionalDoubleEqual(const std::optional<double> &a, const std::optional<double> &b) {
    if (a.has_value() != b.has_value()) {
        return false;
    }
    if (!a.has_value()) {
        return true;
    }
    return *a == *b;
}

SnapshotComparisonResult Fail(const std::string &message) {
    return SnapshotComparisonResult{false, message};
}

} // namespace

SnapshotComparisonResult SnapshotComparator::Compare(const BookSnapshot &lhs,
                                                     const BookSnapshot &rhs) {
    if (lhs.symbol != rhs.symbol) {
        return Fail("symbol mismatch");
    }
    if (lhs.replay_event_index != rhs.replay_event_index) {
        return Fail("replay_event_index mismatch");
    }
    if (lhs.replay_timestamp_ns != rhs.replay_timestamp_ns) {
        return Fail("replay_timestamp_ns mismatch");
    }
    if (lhs.total_events_seen != rhs.total_events_seen) {
        return Fail("total_events_seen mismatch");
    }
    if (lhs.submitted_orders != rhs.submitted_orders) {
        return Fail("submitted_orders mismatch");
    }
    if (lhs.rejected_events != rhs.rejected_events) {
        return Fail("rejected_events mismatch");
    }
    if (lhs.ignored_events != rhs.ignored_events) {
        return Fail("ignored_events mismatch");
    }
    if (lhs.generated_trades != rhs.generated_trades) {
        return Fail("generated_trades mismatch");
    }
    if (!OptionalDoubleEqual(lhs.best_bid, rhs.best_bid)) {
        return Fail("best_bid mismatch");
    }
    if (!OptionalDoubleEqual(lhs.best_ask, rhs.best_ask)) {
        return Fail("best_ask mismatch");
    }
    if (!OptionalDoubleEqual(lhs.mid_price, rhs.mid_price)) {
        return Fail("mid_price mismatch");
    }
    if (!OptionalDoubleEqual(lhs.spread, rhs.spread)) {
        return Fail("spread mismatch");
    }
    if (lhs.bids.size() != rhs.bids.size()) {
        return Fail("bid depth size mismatch");
    }
    if (lhs.asks.size() != rhs.asks.size()) {
        return Fail("ask depth size mismatch");
    }

    for (std::size_t i = 0; i < lhs.bids.size(); ++i) {
        if (lhs.bids[i].price != rhs.bids[i].price ||
            lhs.bids[i].quantity != rhs.bids[i].quantity) {
            std::ostringstream oss;
            oss << "bid depth mismatch at index " << i;
            return Fail(oss.str());
        }
    }

    for (std::size_t i = 0; i < lhs.asks.size(); ++i) {
        if (lhs.asks[i].price != rhs.asks[i].price ||
            lhs.asks[i].quantity != rhs.asks[i].quantity) {
            std::ostringstream oss;
            oss << "ask depth mismatch at index " << i;
            return Fail(oss.str());
        }
    }

    return SnapshotComparisonResult{true, ""};
}

} // namespace bookforge