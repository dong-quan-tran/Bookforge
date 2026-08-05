#pragma once

#include <string>
#include <vector>

#include "HyperliquidCsvReader.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"
#include "replay/StrategyExperiment.hpp"
#include "replay/StrategyExperimentCsvWriter.hpp"
#include "replay/StrategyExperimentSink.hpp"

namespace bookforge {

class StrategyExperimentRunner {
 public:
  explicit StrategyExperimentRunner(const ReplayConfig& replay_config);

  StrategyExperimentResult RunOnce(const StrategyExperimentConfig& config,
                                   const std::vector<ExternalOrderEvent>& events,
                                   const std::string& order_id,
                                   const std::string& participant_id,
                                   StrategyExperimentSink* sink = nullptr) const;

 private:
  ReplayConfig replay_config_;
};

}  // namespace bookforge