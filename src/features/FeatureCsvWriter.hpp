#pragma once

#include <string>
#include <vector>

#include "features/FeatureRow.hpp"

namespace bookforge {

class FeatureCsvWriter {
  public:
    static void Write(const std::string &path, const std::vector<FeatureRow> &rows);
};

} // namespace bookforge