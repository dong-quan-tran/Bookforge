#pragma once

#include <string>
#include <vector>

#include "replay/StrategyExperiment.hpp"

namespace bookforge {

class StrategyExperimentCsvWriter {
  public:
    static bool Write(const std::string &path,
                      const std::vector<StrategyExperimentResult> &results);
};

} // namespace bookforge