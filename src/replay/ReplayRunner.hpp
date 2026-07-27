#pragma once

#include <vector>

#include "ExternalOrderEvent.hpp"
#include "IReplayAdapter.hpp"
#include "ReplayConfig.hpp"

namespace bookforge {

class ReplayRunner {
  public:
    explicit ReplayRunner(const ReplayConfig &config) : config_(config) {}

    bool Run(IReplayAdapter &adapter, const std::vector<ExternalOrderEvent> &events) const;

  private:
    ReplayConfig config_;
};

} // namespace bookforge