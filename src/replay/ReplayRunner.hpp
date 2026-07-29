#pragma once

#include <vector>

#include "ExternalOrderEvent.hpp"
#include "IReplayAdapter.hpp"
#include "ReplayConfig.hpp"
#include "replay/InjectedOrderSchedule.hpp"

namespace bookforge {

class ReplayRunner {
  public:
    explicit ReplayRunner(const ReplayConfig &config) : config_(config) {}

    bool Run(IReplayAdapter &adapter, const std::vector<ExternalOrderEvent> &events) const;

    bool Run(IReplayAdapter &adapter,
             const std::vector<ExternalOrderEvent> &events,
             const InjectedOrderSchedule &schedule) const;

  private:
    ReplayConfig config_;
};

} // namespace bookforge