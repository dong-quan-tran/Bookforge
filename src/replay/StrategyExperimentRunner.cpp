#include "replay/StrategyExperimentRunner.hpp"

#include <utility>

namespace bookforge {

StrategyExperimentRunner::StrategyExperimentRunner(
    const ReplayConfig& replay_config)
    : replay_config_(replay_config) {}

StrategyExperimentResult StrategyExperimentRunner::RunOnce(
    const StrategyExperimentConfig& config,
    const std::vector<ExternalOrderEvent>& events,
    const std::string& order_id,
    const std::string& participant_id,
    StrategyExperimentSink* sink) const {
  ReplayRunner runner(replay_config_);
  const auto injected_order = MakeInjectedOrder(config, order_id, participant_id);
  const auto schedule = MakeSingleOrderSchedule(injected_order);

  StrategyExperimentResult result;
  result.mode = config.mode;
  result.entry_offset = config.entry_offset;
  result.is_buy = config.is_buy;
  result.limit_price = config.limit_price;
  result.requested_qty = config.quantity;

  (void)runner;
  (void)events;
  (void)schedule;

  if (sink != nullptr) {
    sink->OnResult(result);
  }

  return result;
}

}  // namespace bookforge