#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace bookforge {

enum class InjectedOrderTiming { BeforeEvent, AfterEvent };

struct InjectedOrder {
    std::size_t trigger_event_index{0};
    InjectedOrderTiming timing{InjectedOrderTiming::BeforeEvent};

    std::string order_id;
    std::string participant_id;
    std::string symbol;

    bool is_buy{true};
    double price{0.0};
    std::uint32_t quantity{0};
};

} // namespace bookforge
