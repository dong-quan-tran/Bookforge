#pragma once

#include "replay/StrategyExperiment.hpp"

namespace bookforge {

class StrategyExperimentSink {
  public:
    virtual ~StrategyExperimentSink() = default;
    virtual void OnResult(const StrategyExperimentResult &result) = 0;
};

} // namespace bookforge