#pragma once

#include <string>
#include <vector>

#include "HyperliquidCsvReader.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/StrategyExperiment.hpp"

namespace bookforge {

struct StrategyExperimentComparisonResult {
    StrategyExperimentResult passive;
    StrategyExperimentResult aggressive;
};

class StrategyExperimentComparisonRunner {
  public:
    explicit StrategyExperimentComparisonRunner(const ReplayConfig &replay_config);

    bool Run(const StrategyExperimentConfig &passive_config,
             const StrategyExperimentConfig &aggressive_config,
             const std::vector<ExternalOrderEvent> &events, const std::string &passive_order_id,
             const std::string &aggressive_order_id, const std::string &participant_id,
             StrategyExperimentComparisonResult &result) const;

  private:
    ReplayConfig replay_config_;
};

} // namespace bookforge