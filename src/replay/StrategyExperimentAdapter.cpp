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
    result_.implementation_shortfall_bps = 0.0;
    result_.time_to_first_fill_us = 0;
    result_.time_to_full_fill_us = 0;
}

void StrategyExperimentAdapter::OnEvent(const ExternalOrderEvent &event) {
    (void)event;
    // Placeholder: real implementation will track mid, spread, and fills.
}

void StrategyExperimentAdapter::OnInjectedOrder(const InjectedOrder &order) {
    (void)order;
    // Placeholder: real implementation will inject into matching engine
    // and update result_ based on fills.
}

const ReplayMetrics &StrategyExperimentAdapter::Metrics() const {
    return metrics_;
}

StrategyExperimentResult StrategyExperimentAdapter::Result() const {
    return result_;
}

} // namespace bookforge