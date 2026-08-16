#include "replay/StrategyExperimentComparison.hpp"

#include "replay/StrategyExperimentRunner.hpp"

namespace bookforge {

StrategyExperimentComparisonRunner::StrategyExperimentComparisonRunner(
    const ReplayConfig &replay_config)
    : replay_config_(replay_config) {}

bool StrategyExperimentComparisonRunner::Run(const StrategyExperimentConfig &passive_config,
                                             const StrategyExperimentConfig &aggressive_config,
                                             const std::vector<ExternalOrderEvent> &events,
                                             const std::string &passive_order_id,
                                             const std::string &aggressive_order_id,
                                             const std::string &participant_id,
                                             StrategyExperimentComparisonResult &result) const {
    if (passive_config.mode != StrategyMode::Passive ||
        aggressive_config.mode != StrategyMode::Aggressive) {
        return false;
    }

    if (passive_config.entry_offset != aggressive_config.entry_offset) {
        return false;
    }

    StrategyExperimentRunner runner(replay_config_);

    result.passive = runner.RunOnce(passive_config, events, passive_order_id, participant_id);
    result.aggressive =
        runner.RunOnce(aggressive_config, events, aggressive_order_id, participant_id);

    return true;
}

} // namespace bookforge