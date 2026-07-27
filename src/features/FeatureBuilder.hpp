#pragma once

#include <cstddef>
#include <vector>

#include "features/FeatureRow.hpp"
#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

class FeatureBuilder {
  public:
    [[nodiscard]] static std::vector<FeatureRow>
    BuildFromSnapshots(const std::vector<BookSnapshot> &snapshots,
                       std::size_t depth_levels_for_imbalance);

  private:
    [[nodiscard]] static FeatureRow BuildOne(const BookSnapshot &snapshot,
                                             std::size_t depth_levels_for_imbalance);
    [[nodiscard]] static std::optional<double> ComputeDepthImbalance(double bid_qty,
                                                                     double ask_qty);
};

} // namespace bookforge