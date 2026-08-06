#pragma once

#include "HyperliquidMatchingEngineAdapter.hpp"
#include "replay/StrategyExperiment.hpp"

namespace bookforge {

class StrategyExperimentAdapter final : public IReplayAdapter {
  public:
    explicit StrategyExperimentAdapter(StrategyExperimentConfig config);

    void OnEvent(const ExternalOrderEvent &event) override;
    void OnInjectedOrder(const InjectedOrder &order) override;

    const AdapterMetrics &Metrics() const override;

    StrategyExperimentResult Result() const;

    // Hook the replay/matching engine can call when a fill occurs.
    void OnFill(std::uint32_t fill_qty, double fill_price);

  private:
    void MaybeCaptureDecisionMetrics(const ExternalOrderEvent &event);

    StrategyExperimentConfig config_;
    AdapterMetrics metrics_;
    StrategyExperimentResult result_;
};

} // namespace bookforge