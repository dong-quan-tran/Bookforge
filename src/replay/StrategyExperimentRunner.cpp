#include "replay/StrategyExperimentRunner.hpp"

namespace bookforge {

StrategyExperimentRunner::StrategyExperimentRunner(
    const ReplayConfig& replay_config)
    : replay_config_(replay_config) {}

StrategyExperimentResult StrategyExperimentRunner::RunOnce(
    const StrategyExperimentConfig& config,
    const std::vector<ExternalOrderEvent>& events,
    const std::string& order_id,
    const std::string& participant_id) const {
  ReplayRunner runner(replay_config_);
  const auto injected_order =
      MakeInjectedOrder(config, order_id, participant_id);
  const auto schedule = MakeSingleOrderSchedule(injected_order);

  (void)runner;
  (void)events;
  (void)schedule;

  StrategyExperimentResult result;
  result.mode = config.mode;
  result.entry_offset = config.entry_offset;
  result.is_buy = config.is_buy;
  result.limit_price = config.limit_price;
  result.requested_qty = config.quantity;
  result.filled_qty = 0;
  result.remaining_qty = config.quantity;
  result.fill_rate = 0.0;
  result.avg_execution_price = 0.0;
  result.decision_mid_price = 0.0;
  result.decision_spread = 0.0;
  result.implementation_shortfall_bps = 0.0;
  result.time_to_first_fill_us = 0;
  result.time_to_full_fill_us = 0;

  return result;
}

}  // namespace bookforge