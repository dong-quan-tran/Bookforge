#include "snapshot/SnapshotSerializer.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace bookforge {
namespace {

std::string OptionalDoubleToCell(const std::optional<double> &value) {
    if (!value.has_value()) {
        return "";
    }
    std::ostringstream oss;
    oss << *value;
    return oss.str();
}

} // namespace

void SnapshotSerializer::WriteCsv(const std::string &path,
                                  const std::vector<BookSnapshot> &snapshots,
                                  std::size_t depth_levels) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("failed to open snapshot CSV for writing: " + path);
    }

    out << Header(depth_levels) << '\n';
    for (const auto &snapshot : snapshots) {
        out << Row(snapshot, depth_levels) << '\n';
    }
}

std::string SnapshotSerializer::Header(std::size_t depth_levels) {
    std::ostringstream oss;
    oss << "symbol"
        << ",replay_event_index"
        << ",replay_timestamp_ns"
        << ",total_events_seen"
        << ",submitted_orders"
        << ",rejected_events"
        << ",ignored_events"
        << ",generated_trades"
        << ",best_bid"
        << ",best_ask"
        << ",mid_price"
        << ",spread";

    for (std::size_t i = 1; i <= depth_levels; ++i) {
        oss << ",bid_px_" << i << ",bid_qty_" << i;
    }
    for (std::size_t i = 1; i <= depth_levels; ++i) {
        oss << ",ask_px_" << i << ",ask_qty_" << i;
    }

    return oss.str();
}

std::string SnapshotSerializer::Row(const BookSnapshot &snapshot, std::size_t depth_levels) {
    std::ostringstream oss;
    oss << snapshot.symbol << ',' << snapshot.replay_event_index << ','
        << snapshot.replay_timestamp_ns << ',' << snapshot.total_events_seen << ','
        << snapshot.submitted_orders << ',' << snapshot.rejected_events << ','
        << snapshot.ignored_events << ',' << snapshot.generated_trades << ','
        << OptionalDoubleToCell(snapshot.best_bid) << ',' << OptionalDoubleToCell(snapshot.best_ask)
        << ',' << OptionalDoubleToCell(snapshot.mid_price) << ','
        << OptionalDoubleToCell(snapshot.spread);

    for (std::size_t i = 0; i < depth_levels; ++i) {
        if (i < snapshot.bids.size()) {
            oss << ',' << snapshot.bids[i].price << ',' << snapshot.bids[i].quantity;
        } else {
            oss << ',' << "" << ',' << "";
        }
    }

    for (std::size_t i = 0; i < depth_levels; ++i) {
        if (i < snapshot.asks.size()) {
            oss << ',' << snapshot.asks[i].price << ',' << snapshot.asks[i].quantity;
        } else {
            oss << ',' << "" << ',' << "";
        }
    }

    return oss.str();
}

} // namespace bookforge