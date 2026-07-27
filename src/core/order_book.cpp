#include "order_book.hpp"

namespace bookforge {

bool OrderBook::AddOrder(const Order &order) {
    if (order_index_.find(order.id) != order_index_.end()) {
        return false;
    }

    if (order.side == Side::Buy) {
        auto it = bids_.find(order.price);
        if (it == bids_.end()) {
            auto [inserted_it, inserted] = bids_.emplace(order.price, PriceLevel(order.price));
            auto order_it = inserted_it->second.AddOrder(order);
            order_index_[order.id] = OrderLocation{order.side, order.price, order_it};
        } else {
            auto order_it = it->second.AddOrder(order);
            order_index_[order.id] = OrderLocation{order.side, order.price, order_it};
        }
    } else {
        auto it = asks_.find(order.price);
        if (it == asks_.end()) {
            auto [inserted_it, inserted] = asks_.emplace(order.price, PriceLevel(order.price));
            auto order_it = inserted_it->second.AddOrder(order);
            order_index_[order.id] = OrderLocation{order.side, order.price, order_it};
        } else {
            auto order_it = it->second.AddOrder(order);
            order_index_[order.id] = OrderLocation{order.side, order.price, order_it};
        }
    }

    return true;
}

bool OrderBook::CancelOrder(std::uint64_t order_id) {
    auto index_it = order_index_.find(order_id);
    if (index_it == order_index_.end()) {
        return false;
    }

    const auto side = index_it->second.side;
    const auto price = index_it->second.price;
    const auto order_it = index_it->second.order_it;

    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it == bids_.end()) {
            return false;
        }

        it->second.RemoveOrder(order_it);

        if (it->second.Empty()) {
            bids_.erase(it);
        }

        order_index_.erase(index_it);
        return true;
    }

    auto it = asks_.find(price);
    if (it == asks_.end()) {
        return false;
    }

    it->second.RemoveOrder(order_it);

    if (it->second.Empty()) {
        asks_.erase(it);
    }

    order_index_.erase(index_it);
    return true;
}

bool OrderBook::ExecuteTopOrder(Side side, double price, std::uint32_t quantity) {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it == bids_.end()) {
            return false;
        }

        auto front_order = it->second.FrontOrder();
        if (!front_order.has_value()) {
            return false;
        }

        const bool fully_consumed = quantity >= front_order->quantity;

        bool ok = it->second.ReduceFrontOrder(quantity);
        if (!ok) {
            return false;
        }

        if (fully_consumed) {
            order_index_.erase(front_order->id);
        }

        if (it->second.Empty()) {
            bids_.erase(it);
        }

        return true;
    }

    auto it = asks_.find(price);
    if (it == asks_.end()) {
        return false;
    }

    auto front_order = it->second.FrontOrder();
    if (!front_order.has_value()) {
        return false;
    }

    const bool fully_consumed = quantity >= front_order->quantity;

    bool ok = it->second.ReduceFrontOrder(quantity);
    if (!ok) {
        return false;
    }

    if (fully_consumed) {
        order_index_.erase(front_order->id);
    }

    if (it->second.Empty()) {
        asks_.erase(it);
    }

    return true;
}

bool OrderBook::ReduceOrderQuantity(std::uint64_t order_id, std::uint32_t new_quantity) {
    auto index_it = order_index_.find(order_id);
    if (index_it == order_index_.end()) {
        return false;
    }

    const auto side = index_it->second.side;
    const auto price = index_it->second.price;
    const auto order_it = index_it->second.order_it;

    if (side == Side::Buy) {
        auto level_it = bids_.find(price);
        if (level_it == bids_.end()) {
            return false;
        }
        return level_it->second.ReduceOrderQuantity(order_it, new_quantity);
    }

    auto level_it = asks_.find(price);
    if (level_it == asks_.end()) {
        return false;
    }
    return level_it->second.ReduceOrderQuantity(order_it, new_quantity);
}

bool OrderBook::ReplaceOrder(std::uint64_t order_id, double new_price, std::uint32_t new_quantity,
                             std::uint64_t new_timestamp) {
    if (new_quantity == 0) {
        return false;
    }

    auto index_it = order_index_.find(order_id);
    if (index_it == order_index_.end()) {
        return false;
    }

    const auto side = index_it->second.side;
    const auto old_price = index_it->second.price;
    const auto order_it = index_it->second.order_it;

    Order replacement = *order_it;
    replacement.price = new_price;
    replacement.quantity = new_quantity;
    replacement.timestamp = new_timestamp;

    if (side == Side::Buy) {
        auto level_it = bids_.find(old_price);
        if (level_it == bids_.end()) {
            return false;
        }

        level_it->second.RemoveOrder(order_it);

        if (level_it->second.Empty()) {
            bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(old_price);
        if (level_it == asks_.end()) {
            return false;
        }

        level_it->second.RemoveOrder(order_it);

        if (level_it->second.Empty()) {
            asks_.erase(level_it);
        }
    }

    order_index_.erase(index_it);

    return AddOrder(replacement);
}

std::optional<double> OrderBook::GetBestBid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<double> OrderBook::GetBestAsk() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::optional<double> OrderBook::GetMidPrice() const {
    auto best_bid = GetBestBid();
    auto best_ask = GetBestAsk();

    if (!best_bid.has_value() || !best_ask.has_value()) {
        return std::nullopt;
    }

    return (*best_bid + *best_ask) / 2.0;
}

std::optional<double> OrderBook::GetSpread() const {
    auto best_bid = GetBestBid();
    auto best_ask = GetBestAsk();

    if (!best_bid.has_value() || !best_ask.has_value()) {
        return std::nullopt;
    }

    return *best_ask - *best_bid;
}

std::optional<Order> OrderBook::PeekBestBidOrder() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->second.FrontOrder();
}

std::optional<Order> OrderBook::PeekBestAskOrder() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->second.FrontOrder();
}

std::size_t OrderBook::BidLevelCount() const {
    return bids_.size();
}

std::size_t OrderBook::AskLevelCount() const {
    return asks_.size();
}

std::optional<std::uint32_t> OrderBook::GetLevelVolume(Side side, double price) const {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it == bids_.end()) {
            return std::nullopt;
        }
        return it->second.TotalVolume();
    }

    auto it = asks_.find(price);
    if (it == asks_.end()) {
        return std::nullopt;
    }

    return it->second.TotalVolume();
}

std::vector<std::pair<double, std::uint32_t>> OrderBook::GetBidDepth(std::size_t levels) const {
    std::vector<std::pair<double, std::uint32_t>> depth;
    depth.reserve(levels);

    std::size_t count = 0;
    for (const auto &[price, level] : bids_) {
        if (count >= levels) {
            break;
        }
        depth.emplace_back(price, level.TotalVolume());
        ++count;
    }

    return depth;
}

std::vector<std::pair<double, std::uint32_t>> OrderBook::GetAskDepth(std::size_t levels) const {
    std::vector<std::pair<double, std::uint32_t>> depth;
    depth.reserve(levels);

    std::size_t count = 0;
    for (const auto &[price, level] : asks_) {
        if (count >= levels) {
            break;
        }
        depth.emplace_back(price, level.TotalVolume());
        ++count;
    }

    return depth;
}

} // namespace bookforge