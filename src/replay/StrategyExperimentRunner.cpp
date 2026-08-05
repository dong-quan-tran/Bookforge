#include "replay/StrategyExperimentRunner.hpp"

namespace bookforge {

StrategyExperimentRunner::StrategyExperimentRunner(const ReplayConfig &replay_config)
    : replay_config_(replay_config) {}

StrategyExperimentResult StrategyExperimentRunner::RunOnce(
    const StrategyExperimentConfig &config, const std::vector<ExternalOrderEvent> &events,
    const std::string &order_id, const std::string &participant_id) const {
    ReplayRunner runner(replay_config_);

    const auto injected_order = MakeInjectedOrder(config, order_id, participant_id);
    const auto schedule = MakeSingleOrderSchedule(injected_order);

    StrategyExperimentAdapter adapter(config);
    runner.Run(adapter, events, schedule);

    return adapter.Result();
}

} // namespace bookforge