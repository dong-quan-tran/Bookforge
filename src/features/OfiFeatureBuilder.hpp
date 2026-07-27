#pragma once

#include <cstddef>
#include <vector>

#include "features/FeatureRow.hpp"
#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

class OfiFeatureBuilder {
public:

private:
    [[nodiscard]] static double ComputeBestLevelOfi(
        const BookSnapshot& prev,
        const BookSnapshot& curr);

    [[nodiscard]] static double ComputeMultiLevelOfi(
        const BookSnapshot& prev,
        const BookSnapshot& curr,
        std::size_t depth_levels_for_ofi);

    [[nodiscard]] static double ComputeWeightedMultiLevelOfi(
        const BookSnapshot& prev,
        const BookSnapshot& curr,
        std::size_t depth_levels_for_ofi);

    [[nodiscard]] static double ComputeSideOfiBest(
        const std::optional<DepthLevelSnapshot>& prev_level,
        const std::optional<DepthLevelSnapshot>& curr_level,
        bool is_bid_side);

    [[nodiscard]] static double ComputeSideOfiMulti(
        const std::vector<DepthLevelSnapshot>& prev_side,
        const std::vector<DepthLevelSnapshot>& curr_side,
        std::size_t depth_levels,
        bool is_bid_side);

    [[nodiscard]] static double ComputeSideOfiWeighted(
        const std::vector<DepthLevelSnapshot>& prev_side,
        const std::vector<DepthLevelSnapshot>& curr_side,
        std::size_t depth_levels,
        bool is_bid_side);

    [[nodiscard]] static double WeightForLevel(std::size_t zero_based_level);
};

}  // namespace bookforge