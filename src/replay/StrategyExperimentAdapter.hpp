#pragma once

#include <cstdint>

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

    void OnFill(std::uint32_t fill_qty, double fill_price, std::uint64_t fill_timestamp_ns);
    void CaptureDecisionBookState(const TopOfBookSnapshot &snapshot);
    void RecordInjectionTimestamp(std::uint64_t injection_timestamp_ns);

  private:
    StrategyExperimentConfig config_;
    AdapterMetrics metrics_;
    StrategyExperimentResult result_;
    std::uint64_t injection_timestamp_ns_{0};
    bool injection_timestamp_recorded_{false};
    bool decision_snapshot_captured_{false};
    bool first_fill_timestamp_recorded_{false};
    bool full_fill_timestamp_recorded_{false};
};

} // namespace bookforge
