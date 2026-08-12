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

    void OnFill(std::uint32_t fill_qty, double fill_price);
    void CaptureDecisionBookState(const TopOfBookSnapshot &snapshot);

  private:
    StrategyExperimentConfig config_;
    AdapterMetrics metrics_;
    StrategyExperimentResult result_;
    bool decision_snapshot_captured_{false};
};

} // namespace bookforge