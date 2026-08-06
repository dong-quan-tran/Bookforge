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

  private:
    void MaybeCaptureDecisionMetrics(const ExternalOrderEvent &event);

    StrategyExperimentConfig config_;
    AdapterMetrics metrics_;
    StrategyExperimentResult result_;
};

} // namespace bookforge