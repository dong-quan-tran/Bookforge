#include "price_level.hpp"

namespace bookforge {

PriceLevel::PriceLevel(double price) : price_(price) {}

PriceLevel::Iterator PriceLevel::AddOrder(const Order &order) {
    orders_.push_back(order);
    auto it = orders_.end();
    --it;
    return it;
}

void PriceLevel::RemoveOrder(Iterator it) {
    orders_.erase(it);
}

std::optional<Order> PriceLevel::PopFrontOrder() {
    if (orders_.empty()) {
        return std::nullopt;
    }

    Order front = orders_.front();
    orders_.pop_front();
    return front;
}

bool PriceLevel::ReduceFrontOrder(std::uint32_t quantity) {
    if (orders_.empty()) {
        return false;
    }

    if (quantity >= orders_.front().quantity) {
        orders_.pop_front();
        return true;
    }

    orders_.front().quantity -= quantity;
    return true;
}

bool PriceLevel::ReduceOrderQuantity(Iterator it, std::uint32_t new_quantity) {
    if (it == orders_.end()) {
        return false;
    }

    if (new_quantity == 0) {
        return false;
    }

    if (new_quantity >= it->quantity) {
        return false;
    }

    it->quantity = new_quantity;
    return true;
}

std::optional<Order> PriceLevel::FrontOrder() const {
    if (orders_.empty()) {
        return std::nullopt;
    }
    return orders_.front();
}

double PriceLevel::GetPrice() const {
    return price_;
}

std::uint32_t PriceLevel::TotalVolume() const {
    std::uint32_t total = 0;
    for (const auto &order : orders_) {
        total += order.quantity;
    }
    return total;
}

bool PriceLevel::Empty() const {
    return orders_.empty();
}

std::size_t PriceLevel::OrderCount() const {
    return orders_.size();
}

} // namespace bookforge