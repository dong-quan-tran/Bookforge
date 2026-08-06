#include "replay/StrategyExperimentAdapter.hpp"

namespace bookforge {

StrategyExperimentAdapter::StrategyExperimentAdapter(StrategyExperimentConfig config)
    : config_(std::move(config)) {
    result_.mode = config_.mode;
    result_.entry_offset = config_.entry_offset;
    result_.is_buy = config_.is_buy;
    result_.limit_price = config_.limit_price;
    result_.requested_qty = config_.quantity;
    result_.filled_qty = 0;
    result_.remaining_qty = config_.quantity;
    result_.fill_rate = 0.0;
    result_.avg_execution_price = 0.0;
    result_.decision_mid_price = 0.0;
    result_.decision_spread = 0.0;
    result_.has_decision_metrics = false;
    result_.implementation_shortfall_bps = 0.0;
    result_.time_to_first_fill_us = 0;
    result_.time_to_full_fill_us = 0;
}

void StrategyExperimentAdapter::OnEvent(const ExternalOrderEvent &event) {
    MaybeCaptureDecisionMetrics(event);
    (void)event;
    // Placeholder: real implementation will track fills and execution metrics.
}

void StrategyExperimentAdapter::OnInjectedOrder(const InjectedOrder &order) {
    (void)order;
    // Placeholder: real implementation will inject into matching engine
    // and update result_ based on fills.
}

const AdapterMetrics &StrategyExperimentAdapter::Metrics() const {
    return metrics_;
}

StrategyExperimentResult StrategyExperimentAdapter::Result() const {
    return result_;
}

void StrategyExperimentAdapter::MaybeCaptureDecisionMetrics(const ExternalOrderEvent &event) {
    (void)event;

    // Placeholder implementation:
    // Capture metrics once on the first event processed.
    if (result_.has_decision_metrics) {
        return;
    }

    // In a fuller implementation, this would inspect book state and compute
    // mid/ spread from the matching engine / order book at decision time.
    result_.has_decision_metrics = true;
}

} // namespace bookforge