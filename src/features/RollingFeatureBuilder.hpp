#pragma once

#include <cstddef>
#include <vector>

#include "features/FeatureRow.hpp"

namespace bookforge {

class RollingFeatureBuilder {
  public:
    static void AddRollingContextFeatures(std::vector<FeatureRow> &rows, std::size_t window);
};

} // namespace bookforge