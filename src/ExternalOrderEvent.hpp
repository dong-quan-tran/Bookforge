#pragma once

#include <chrono>
#include <string>

namespace bookforge {

enum class EventType { New, Cancel, Fill, Reject, Trigger, Other };

struct ExternalOrderEvent {
    std::chrono::nanoseconds ts;
    std::string symbol;
    std::string external_order_id;
    double price;
    double size;
    bool isAsk;             // true = ask, false = bid
    int statusId;           // raw Hyperliquid code
    std::string statusText; // e.g. "open", "perpMarginRejected"
    EventType eventType;
};

} // namespace bookforge
