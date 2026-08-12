#include "replay/StrategyExperimentRunner.hpp"

#include <string>

#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/StrategyExperimentAdapter.hpp"

namespace bookforge {
namespace {

class StrategyExperimentReplayAdapter final : public IReplayAdapter {
  public:
    StrategyExperimentReplayAdapter(StrategyExperimentAdapter &experiment_adapter,
                                    HyperliquidMatchingEngineAdapter &matching_adapter,
                                    std::string experiment_order_id)
        : experiment_adapter_(experiment_adapter), matching_adapter_(matching_adapter),
          experiment_order_id_(std::move(experiment_order_id)) {}

    void OnEvent(const ExternalOrderEvent &event) override {
        matching_adapter_.OnEvent(event);
        experiment_adapter_.OnEvent(event);
    }

    void OnInjectedOrder(const InjectedOrder &order) override {
        experiment_adapter_.OnInjectedOrder(order);
        matching_adapter_.OnInjectedOrder(order);
    }

    const AdapterMetrics &Metrics() const override {
        return matching_adapter_.Metrics();
    }

    void OnInjectedOrderFill(const InjectedOrder &order, const Trade &trade) {
        if (order.order_id != experiment_order_id_) {
            return;
        }

        experiment_adapter_.OnFill(trade.quantity, trade.price);
    }

  private:
    StrategyExperimentAdapter &experiment_adapter_;
    HyperliquidMatchingEngineAdapter &matching_adapter_;
    std::string experiment_order_id_;
};

} // namespace

StrategyExperimentRunner::StrategyExperimentRunner(const ReplayConfig &config)
    : replay_config_(config) {}

StrategyExperimentResult
StrategyExperimentRunner::RunOnce(const StrategyExperimentConfig &experiment_config,
                                  const std::vector<ExternalOrderEvent> &events,
                                  const std::string &injected_order_id,
                                  const std::string &experiment_label) const {
    StrategyExperimentAdapter experiment_adapter(experiment_config);
    MatchingEngine engine;

    StrategyExperimentReplayAdapter *replay_adapter_ptr = nullptr;

    HyperliquidMatchingEngineAdapter matching_adapter(
        engine, [&replay_adapter_ptr](const InjectedOrder &order, const Trade &trade) {
            if (replay_adapter_ptr != nullptr) {
                replay_adapter_ptr->OnInjectedOrderFill(order, trade);
            }
        });

    StrategyExperimentReplayAdapter replay_adapter(experiment_adapter, matching_adapter,
                                                   injected_order_id);
    replay_adapter_ptr = &replay_adapter;

    const InjectedOrder injected_order =
        MakeInjectedOrder(experiment_config, injected_order_id, experiment_label);
    const InjectedOrderSchedule schedule = MakeSingleOrderSchedule(injected_order);

    ReplayRunner replay_runner(replay_config_);
    (void)replay_runner.Run(replay_adapter, events, schedule);

    return experiment_adapter.Result();
}

} // namespace bookforge