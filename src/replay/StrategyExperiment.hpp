#pragma once

#include <cstdint>
#include <string>

#include "replay/InjectedOrder.hpp"
#include "replay/InjectedOrderSchedule.hpp"

namespace bookforge {

enum class StrategyMode { Passive, Aggressive };

struct StrategyExperimentConfig {
    StrategyMode mode{StrategyMode::Passive};
    std::string csv_path;
    std::uint64_t entry_offset{0};
    bool is_buy{true};
    double limit_price{0.0};
    std::uint32_t quantity{0};
    InjectedOrderTiming timing{InjectedOrderTiming::BeforeEvent};
};

struct StrategyExperimentResult {
    StrategyMode mode{StrategyMode::Passive};
    std::uint64_t entry_offset{0};
    bool is_buy{true};
    double limit_price{0.0};
    std::uint32_t requested_qty{0};
    std::uint32_t filled_qty{0};
    std::uint32_t remaining_qty{0};

    double fill_rate{0.0};
    double avg_execution_price{0.0};

    double decision_mid_price{0.0};
    double decision_spread{0.0};
    bool has_decision_metrics{false};

    double implementation_shortfall_bps{0.0};

    std::uint64_t time_to_first_fill_us{0};
    std::uint64_t time_to_full_fill_us{0};
};

InjectedOrder MakeInjectedOrder(const StrategyExperimentConfig &config, const std::string &order_id,
                                const std::string &participant_id);

InjectedOrderSchedule MakeSingleOrderSchedule(const InjectedOrder &order);

} // namespace bookforge