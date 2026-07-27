#include "features/OfiFeatureBuilder.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>

namespace bookforge {

void OfiFeatureBuilder::AddOfiFeatures(std::vector<FeatureRow> &rows,
                                       const std::vector<BookSnapshot> &snapshots,
                                       std::size_t depth_levels_for_ofi) {
    if (rows.size() != snapshots.size()) {
        throw std::runtime_error("rows and snapshots size mismatch in OfiFeatureBuilder");
    }

    if (rows.empty()) {
        return;
    }

    rows[0].ofi_l1 = std::nullopt;
    rows[0].ofi_lN = std::nullopt;
    rows[0].weighted_ofi_lN = std::nullopt;

    for (std::size_t i = 1; i < rows.size(); ++i) {
        const auto &prev = snapshots[i - 1];
        const auto &curr = snapshots[i];

        rows[i].ofi_l1 = ComputeBestLevelOfi(prev, curr);
        rows[i].ofi_lN = ComputeMultiLevelOfi(prev, curr, depth_levels_for_ofi);
        rows[i].weighted_ofi_lN = ComputeWeightedMultiLevelOfi(prev, curr, depth_levels_for_ofi);
    }
}

double OfiFeatureBuilder::ComputeBestLevelOfi(const BookSnapshot &prev, const BookSnapshot &curr) {
    const std::optional<DepthLevelSnapshot> prev_bid =
        prev.bids.empty() ? std::optional<DepthLevelSnapshot>{}
                          : std::optional<DepthLevelSnapshot>{prev.bids.front()};
    const std::optional<DepthLevelSnapshot> curr_bid =
        curr.bids.empty() ? std::optional<DepthLevelSnapshot>{}
                          : std::optional<DepthLevelSnapshot>{curr.bids.front()};

    const std::optional<DepthLevelSnapshot> prev_ask =
        prev.asks.empty() ? std::optional<DepthLevelSnapshot>{}
                          : std::optional<DepthLevelSnapshot>{prev.asks.front()};
    const std::optional<DepthLevelSnapshot> curr_ask =
        curr.asks.empty() ? std::optional<DepthLevelSnapshot>{}
                          : std::optional<DepthLevelSnapshot>{curr.asks.front()};

    return ComputeSideOfiBest(prev_bid, curr_bid, true) +
           ComputeSideOfiBest(prev_ask, curr_ask, false);
}

double OfiFeatureBuilder::ComputeMultiLevelOfi(const BookSnapshot &prev, const BookSnapshot &curr,
                                               std::size_t depth_levels_for_ofi) {
    return ComputeSideOfiMulti(prev.bids, curr.bids, depth_levels_for_ofi, true) +
           ComputeSideOfiMulti(prev.asks, curr.asks, depth_levels_for_ofi, false);
}

double OfiFeatureBuilder::ComputeWeightedMultiLevelOfi(const BookSnapshot &prev,
                                                       const BookSnapshot &curr,
                                                       std::size_t depth_levels_for_ofi) {
    return ComputeSideOfiWeighted(prev.bids, curr.bids, depth_levels_for_ofi, true) +
           ComputeSideOfiWeighted(prev.asks, curr.asks, depth_levels_for_ofi, false);
}

double OfiFeatureBuilder::ComputeSideOfiBest(const std::optional<DepthLevelSnapshot> &prev_level,
                                             const std::optional<DepthLevelSnapshot> &curr_level,
                                             bool is_bid_side) {
    if (!prev_level.has_value() && !curr_level.has_value()) {
        return 0.0;
    }

    if (!prev_level.has_value() && curr_level.has_value()) {
        const double v_curr = static_cast<double>(curr_level->quantity);
        return is_bid_side ? +v_curr : -v_curr;
    }

    if (prev_level.has_value() && !curr_level.has_value()) {
        const double v_prev = static_cast<double>(prev_level->quantity);
        return is_bid_side ? -v_prev : +v_prev;
    }

    const double p_prev = prev_level->price;
    const double v_prev = static_cast<double>(prev_level->quantity);
    const double p_curr = curr_level->price;
    const double v_curr = static_cast<double>(curr_level->quantity);

    if (is_bid_side) {
        if (p_curr > p_prev) {
            return +v_curr;
        }
        if (p_curr < p_prev) {
            return -v_prev;
        }
        return v_curr - v_prev;
    }

    if (p_curr < p_prev) {
        return -v_curr;
    }
    if (p_curr > p_prev) {
        return +v_prev;
    }
    return v_prev - v_curr;
}

double OfiFeatureBuilder::ComputeSideOfiMulti(const std::vector<DepthLevelSnapshot> &prev_side,
                                              const std::vector<DepthLevelSnapshot> &curr_side,
                                              std::size_t depth_levels, bool is_bid_side) {
    const std::size_t levels = std::min(depth_levels, std::max(prev_side.size(), curr_side.size()));

    double total = 0.0;
    for (std::size_t i = 0; i < levels; ++i) {
        const std::optional<DepthLevelSnapshot> prev_level =
            (i < prev_side.size()) ? std::optional<DepthLevelSnapshot>{prev_side[i]}
                                   : std::optional<DepthLevelSnapshot>{};

        const std::optional<DepthLevelSnapshot> curr_level =
            (i < curr_side.size()) ? std::optional<DepthLevelSnapshot>{curr_side[i]}
                                   : std::optional<DepthLevelSnapshot>{};

        total += ComputeSideOfiBest(prev_level, curr_level, is_bid_side);
    }

    return total;
}

double OfiFeatureBuilder::ComputeSideOfiWeighted(const std::vector<DepthLevelSnapshot> &prev_side,
                                                 const std::vector<DepthLevelSnapshot> &curr_side,
                                                 std::size_t depth_levels, bool is_bid_side) {
    const std::size_t levels = std::min(depth_levels, std::max(prev_side.size(), curr_side.size()));

    double total = 0.0;
    for (std::size_t i = 0; i < levels; ++i) {
        const std::optional<DepthLevelSnapshot> prev_level =
            (i < prev_side.size()) ? std::optional<DepthLevelSnapshot>{prev_side[i]}
                                   : std::optional<DepthLevelSnapshot>{};

        const std::optional<DepthLevelSnapshot> curr_level =
            (i < curr_side.size()) ? std::optional<DepthLevelSnapshot>{curr_side[i]}
                                   : std::optional<DepthLevelSnapshot>{};

        const double level_ofi = ComputeSideOfiBest(prev_level, curr_level, is_bid_side);
        total += WeightForLevel(i) * level_ofi;
    }

    return total;
}

double OfiFeatureBuilder::WeightForLevel(std::size_t zero_based_level) {
    return 1.0 / static_cast<double>(zero_based_level + 1);
}

} // namespace bookforge