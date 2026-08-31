#include "replay/StrategyExperimentRunner.hpp"

#include <string>
#include <utility>

#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/StrategyExperimentAdapter.hpp"

namespace bookforge {
namespace {

class StrategyExperimentReplayAdapter final : public IReplayAdapter {
  public:
    StrategyExperimentReplayAdapter(StrategyExperimentAdapter &experiment_adapter,
                                    HyperliquidMatchingEngineAdapter &matching_adapter,
                                    MatchingEngine &engine, std::string experiment_order_id)
        : experiment_adapter_(experiment_adapter), matching_adapter_(matching_adapter),
          engine_(engine), experiment_order_id_(std::move(experiment_order_id)) {}

    void OnEvent(const ExternalOrderEvent &event) override {
        matching_adapter_.OnEvent(event);
        experiment_adapter_.OnEvent(event);
    }

    void OnInjectedOrder(const InjectedOrder &order) override {
        if (order.order_id == experiment_order_id_) {
            experiment_adapter_.CaptureDecisionBookState(engine_.CaptureTopOfBook());
            experiment_adapter_.RecordInjectionTimestamp(order.replay_timestamp_ns);
        }

        experiment_adapter_.OnInjectedOrder(order);
        matching_adapter_.OnInjectedOrder(order);
    }

    const AdapterMetrics &Metrics() const override {
        return matching_adapter_.Metrics();
    }

  private:
    StrategyExperimentAdapter &experiment_adapter_;
    HyperliquidMatchingEngineAdapter &matching_adapter_;
    MatchingEngine &engine_;
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

    HyperliquidMatchingEngineAdapter matching_adapter(
        engine,
        [&experiment_adapter, &injected_order_id](const InjectedOrder &order, const Trade &trade) {
            if (order.order_id == injected_order_id) {
                experiment_adapter.OnFill(trade.quantity, trade.price, trade.taker_timestamp);
            }
        });

    StrategyExperimentReplayAdapter replay_adapter(experiment_adapter, matching_adapter, engine,
                                                   injected_order_id);

    const InjectedOrder injected_order =
        MakeInjectedOrder(experiment_config, injected_order_id, experiment_label);
    const InjectedOrderSchedule schedule = MakeSingleOrderSchedule(injected_order);

    ReplayRunner replay_runner(replay_config_);
    (void)replay_runner.Run(replay_adapter, events, schedule);

    return experiment_adapter.Result();
}

} // namespace bookforge
