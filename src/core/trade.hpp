#pragma once

#include <cstdint>

#include "order.hpp"

namespace bookforge {

struct Trade {
    std::uint64_t taker_order_id; // incoming order
    std::uint64_t maker_order_id; // resting order in the book
    Side side;                    // side of the taker order
    double price;
    std::uint32_t quantity;
    std::uint64_t taker_timestamp{0};
};

} // namespace bookforge