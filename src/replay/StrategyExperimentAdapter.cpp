#include "replay/StrategyExperimentAdapter.hpp"

#include <algorithm>

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
    // Real implementation will track fills and execution metrics via OnFill.
}

void StrategyExperimentAdapter::OnInjectedOrder(const InjectedOrder &order) {
    (void)order;
    // Real implementation will inject into matching engine and update result_
    // based on fills via OnFill.
}

const AdapterMetrics &StrategyExperimentAdapter::Metrics() const {
    return metrics_;
}

StrategyExperimentResult StrategyExperimentAdapter::Result() const {
    StrategyExperimentResult copy = result_;

    if (copy.requested_qty > 0) {
        copy.fill_rate =
            static_cast<double>(copy.filled_qty) / static_cast<double>(copy.requested_qty);
    } else {
        copy.fill_rate = 0.0;
    }

    return copy;
}

void StrategyExperimentAdapter::OnFill(std::uint32_t fill_qty, double fill_price) {
    if (fill_qty == 0) {
        return;
    }

    const auto new_filled = static_cast<std::uint32_t>(
        std::min<std::uint32_t>(result_.filled_qty + fill_qty, result_.requested_qty));
    const auto actual_fill = new_filled - result_.filled_qty;

    result_.filled_qty = new_filled;
    if (result_.requested_qty >= result_.filled_qty) {
        result_.remaining_qty = result_.requested_qty - result_.filled_qty;
    } else {
        result_.remaining_qty = 0;
    }

    if (actual_fill > 0) {
        const double previous_notional =
            result_.avg_execution_price * static_cast<double>(result_.filled_qty - actual_fill);
        const double new_notional =
            previous_notional + fill_price * static_cast<double>(actual_fill);

        result_.avg_execution_price = new_notional / static_cast<double>(result_.filled_qty);
    }
}

void StrategyExperimentAdapter::MaybeCaptureDecisionMetrics(const ExternalOrderEvent &event) {
    (void)event;

    // Placeholder implementation:
    // Capture metrics once on the first event processed.
    if (result_.has_decision_metrics) {
        return;
    }

    // In a fuller implementation, this would inspect book state and compute
    // mid/spread from the matching engine / order book at decision time.
    result_.has_decision_metrics = true;
}

} // namespace bookforge