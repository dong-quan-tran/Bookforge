#include "features/FeatureBuilder.hpp"

#include <algorithm>

namespace bookforge {

std::vector<FeatureRow>
FeatureBuilder::BuildFromSnapshots(const std::vector<BookSnapshot> &snapshots,
                                   std::size_t depth_levels_for_imbalance) {
    std::vector<FeatureRow> rows;
    rows.reserve(snapshots.size());

    for (const auto &snapshot : snapshots) {
        rows.push_back(BuildOne(snapshot, depth_levels_for_imbalance));
    }

    return rows;
}

FeatureRow FeatureBuilder::BuildOne(const BookSnapshot &snapshot,
                                    std::size_t depth_levels_for_imbalance) {
    FeatureRow row{};
    row.symbol = snapshot.symbol;
    row.replay_event_index = snapshot.replay_event_index;
    row.replay_timestamp_ns = snapshot.replay_timestamp_ns;

    row.best_bid = snapshot.best_bid;
    row.best_ask = snapshot.best_ask;
    row.spread = snapshot.spread;
    row.mid_price = snapshot.mid_price;

    if (!snapshot.bids.empty()) {
        row.l1_bid_qty = static_cast<double>(snapshot.bids.front().quantity);
    }
    if (!snapshot.asks.empty()) {
        row.l1_ask_qty = static_cast<double>(snapshot.asks.front().quantity);
    }

    if (row.l1_bid_qty.has_value() && row.l1_ask_qty.has_value()) {
        row.l1_depth_imbalance = ComputeDepthImbalance(*row.l1_bid_qty, *row.l1_ask_qty);
    }

    const std::size_t bid_levels = std::min(depth_levels_for_imbalance, snapshot.bids.size());
    const std::size_t ask_levels = std::min(depth_levels_for_imbalance, snapshot.asks.size());

    double bid_sum = 0.0;
    for (std::size_t i = 0; i < bid_levels; ++i) {
        bid_sum += static_cast<double>(snapshot.bids[i].quantity);
    }

    double ask_sum = 0.0;
    for (std::size_t i = 0; i < ask_levels; ++i) {
        ask_sum += static_cast<double>(snapshot.asks[i].quantity);
    }

    if (bid_levels > 0) {
        row.lN_bid_qty_sum = bid_sum;
    }
    if (ask_levels > 0) {
        row.lN_ask_qty_sum = ask_sum;
    }

    if (row.lN_bid_qty_sum.has_value() && row.lN_ask_qty_sum.has_value()) {
        row.lN_depth_imbalance = ComputeDepthImbalance(*row.lN_bid_qty_sum, *row.lN_ask_qty_sum);
    }

    return row;
}

std::optional<double> FeatureBuilder::ComputeDepthImbalance(double bid_qty, double ask_qty) {
    const double denom = bid_qty + ask_qty;
    if (denom <= 0.0) {
        return std::nullopt;
    }
    return (bid_qty - ask_qty) / denom;
}

} // namespace bookforge