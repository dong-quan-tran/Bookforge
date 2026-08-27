#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace bookforge {

enum class EventType { New, Cancel, Fill, Replace, Reject, Trigger, Other };

struct ExternalOrderEvent {
    std::chrono::nanoseconds ts;
    std::string symbol;
    std::string external_order_id;
    std::optional<double> external_fill_size;
    double price;
    double size;
    bool isAsk;             // true = ask, false = bid
    int statusId;           // raw Hyperliquid code
    std::string statusText; // e.g. "open", "replaced"
    EventType eventType;
};

} // namespace bookforge
