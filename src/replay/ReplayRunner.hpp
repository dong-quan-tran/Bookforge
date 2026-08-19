#pragma once

#include <memory>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "IReplayAdapter.hpp"
#include "ReplayConfig.hpp"
#include "replay/InjectedOrderSchedule.hpp"
#include "replay/ReplayClock.hpp"

namespace bookforge {

class ReplayRunner {
  public:
    explicit ReplayRunner(const ReplayConfig &config);
    ReplayRunner(const ReplayConfig &config, IReplayClock &clock);

    bool Run(IReplayAdapter &adapter, const std::vector<ExternalOrderEvent> &events) const;

    bool Run(IReplayAdapter &adapter, const std::vector<ExternalOrderEvent> &events,
             const InjectedOrderSchedule &schedule) const;

  private:
    ReplayConfig config_;
    std::unique_ptr<IReplayClock> owned_clock_;
    IReplayClock *clock_{nullptr};
};

} // namespace bookforge