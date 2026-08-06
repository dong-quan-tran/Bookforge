#include "replay/StrategyExperimentRunner.hpp"

#include "replay/StrategyExperimentAdapter.hpp"

namespace bookforge {

StrategyExperimentRunner::StrategyExperimentRunner(const ReplayConfig &config)
    : replay_config_(config) {}

StrategyExperimentResult
StrategyExperimentRunner::RunOnce(const StrategyExperimentConfig &experiment_config,
                                  const std::vector<ExternalOrderEvent> &events,
                                  const std::string &injected_order_id,
                                  const std::string &experiment_label) const {
    (void)injected_order_id;
    (void)experiment_label;

    // Construct the adapter with the experiment config.
    StrategyExperimentAdapter adapter(experiment_config);

    // Feed events through the adapter. For now we only capture decision-time
    // metrics and leave fill tracking to be wired later via OnFill().
    for (const auto &e : events) {
        adapter.OnEvent(e);
    }

    // Return the adapter's result.
    return adapter.Result();
}

} // namespace bookforge