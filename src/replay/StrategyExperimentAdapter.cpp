#include "replay/StrategyExperimentAdapter.hpp"

#include <algorithm>
#include <utility>

namespace bookforge {
namespace {

std::uint64_t ElapsedMicroseconds(std::uint64_t start_timestamp_ns,
                                  std::uint64_t end_timestamp_ns) {
    if (end_timestamp_ns <= start_timestamp_ns) {
        return 0;
    }

    return (end_timestamp_ns - start_timestamp_ns) / 1000;
}

double ComputeImplementationShortfallBps(const StrategyExperimentResult &result) {
    if (!result.has_decision_metrics || result.filled_qty == 0 ||
        result.decision_mid_price <= 0.0) {
        return 0.0;
    }

    if (result.is_buy) {
        return (result.avg_execution_price - result.decision_mid_price) /
               result.decision_mid_price * 10000.0;
    }

    return (result.decision_mid_price - result.avg_execution_price) / result.decision_mid_price *
           10000.0;
}

} // namespace

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
    result_.decision_best_bid.reset();
    result_.decision_best_ask.reset();
    result_.decision_mid_price = 0.0;
    result_.decision_spread = 0.0;
    result_.has_decision_metrics = false;
    result_.implementation_shortfall_bps = 0.0;
    result_.time_to_first_fill_us = 0;
    result_.time_to_full_fill_us = 0;
}

void StrategyExperimentAdapter::OnEvent(const ExternalOrderEvent &) {}

void StrategyExperimentAdapter::OnInjectedOrder(const InjectedOrder &) {}

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

    copy.implementation_shortfall_bps = ComputeImplementationShortfallBps(copy);

    return copy;
}

void StrategyExperimentAdapter::OnFill(std::uint32_t fill_qty, double fill_price,
                                       std::uint64_t fill_timestamp_ns) {
    if (fill_qty == 0) {
        return;
    }

    const std::uint32_t filled_before = result_.filled_qty;
    const std::uint32_t new_filled = static_cast<std::uint32_t>(
        std::min<std::uint32_t>(filled_before + fill_qty, result_.requested_qty));
    const std::uint32_t actual_fill = new_filled - filled_before;

    if (actual_fill == 0) {
        return;
    }

    result_.filled_qty = new_filled;
    result_.remaining_qty = result_.requested_qty - result_.filled_qty;

    const double previous_notional =
        result_.avg_execution_price * static_cast<double>(filled_before);
    const double new_notional = previous_notional + fill_price * static_cast<double>(actual_fill);

    result_.avg_execution_price = new_notional / static_cast<double>(result_.filled_qty);

    if (injection_timestamp_recorded_ && !first_fill_timestamp_recorded_) {
        result_.time_to_first_fill_us =
            ElapsedMicroseconds(injection_timestamp_ns_, fill_timestamp_ns);
        first_fill_timestamp_recorded_ = true;
    }

    if (injection_timestamp_recorded_ && result_.remaining_qty == 0 &&
        !full_fill_timestamp_recorded_) {
        result_.time_to_full_fill_us =
            ElapsedMicroseconds(injection_timestamp_ns_, fill_timestamp_ns);
        full_fill_timestamp_recorded_ = true;
    }
}

void StrategyExperimentAdapter::CaptureDecisionBookState(const TopOfBookSnapshot &snapshot) {
    if (decision_snapshot_captured_) {
        return;
    }

    decision_snapshot_captured_ = true;

    result_.decision_best_bid = snapshot.best_bid;
    result_.decision_best_ask = snapshot.best_ask;

    if (snapshot.mid_price.has_value()) {
        result_.decision_mid_price = *snapshot.mid_price;
    }

    if (snapshot.spread.has_value()) {
        result_.decision_spread = *snapshot.spread;
    }

    result_.has_decision_metrics = snapshot.best_bid.has_value() && snapshot.best_ask.has_value() &&
                                   snapshot.mid_price.has_value() && snapshot.spread.has_value();
}

void StrategyExperimentAdapter::RecordInjectionTimestamp(std::uint64_t injection_timestamp_ns) {
    if (injection_timestamp_recorded_) {
        return;
    }

    injection_timestamp_ns_ = injection_timestamp_ns;
    injection_timestamp_recorded_ = true;
}

} // namespace bookforge
