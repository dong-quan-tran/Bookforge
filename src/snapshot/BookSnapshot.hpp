#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace bookforge {

struct DepthLevelSnapshot {
    double price{0.0};
    std::uint32_t quantity{0};
};

struct BookSnapshot {
    std::string symbol;
    std::uint64_t replay_event_index{0};
    std::uint64_t replay_timestamp_ns{0};

    std::uint64_t total_events_seen{0};
    std::uint64_t submitted_orders{0};
    std::uint64_t rejected_events{0};
    std::uint64_t ignored_events{0};
    std::uint64_t generated_trades{0};

    std::optional<double> best_bid;
    std::optional<double> best_ask;
    std::optional<double> mid_price;
    std::optional<double> spread;

    std::vector<DepthLevelSnapshot> bids;
    std::vector<DepthLevelSnapshot> asks;
};

} // namespace bookforge