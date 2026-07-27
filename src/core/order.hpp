#pragma once

#include <cstdint>

namespace bookforge {

enum class Side : std::uint8_t { Buy = 0, Sell = 1 };

enum class SelfTradePrevention { None, CancelNewest, CancelOldest };

struct Order {
    std::uint64_t id{};
    std::uint64_t participant_id{0};
    Side side{Side::Buy};
    double price{0.0};
    std::uint32_t quantity{0};
    std::uint64_t timestamp{0};
    SelfTradePrevention stp{SelfTradePrevention::None};
};

} // namespace bookforge