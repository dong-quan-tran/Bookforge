#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "order.hpp"
#include "price_level.hpp"

namespace bookforge {

struct OrderLocation {
    Side side{Side::Buy};
    double price{0.0};
    PriceLevel::Iterator order_it;
};

class OrderBook {
  public:
    bool AddOrder(const Order &order);
    bool CancelOrder(std::uint64_t order_id);
    bool ExecuteTopOrder(Side side, double price, std::uint32_t quantity);

    bool ReduceOrderQuantity(std::uint64_t order_id, std::uint32_t new_quantity);
    bool ReplaceOrder(std::uint64_t order_id, double new_price, std::uint32_t new_quantity,
                      std::uint64_t new_timestamp);

    [[nodiscard]] std::optional<double> GetBestBid() const;
    [[nodiscard]] std::optional<double> GetBestAsk() const;
    [[nodiscard]] std::optional<double> GetMidPrice() const;
    [[nodiscard]] std::optional<double> GetSpread() const;

    [[nodiscard]] std::size_t BidLevelCount() const;
    [[nodiscard]] std::size_t AskLevelCount() const;

    [[nodiscard]] std::optional<std::uint32_t> GetLevelVolume(Side side, double price) const;
    [[nodiscard]] std::vector<std::pair<double, std::uint32_t>>
    GetBidDepth(std::size_t levels) const;
    [[nodiscard]] std::vector<std::pair<double, std::uint32_t>>
    GetAskDepth(std::size_t levels) const;

    [[nodiscard]] std::optional<Order> PeekBestBidOrder() const;
    [[nodiscard]] std::optional<Order> PeekBestAskOrder() const;

  private:
    std::map<double, PriceLevel, std::greater<>> bids_;
    std::map<double, PriceLevel, std::less<>> asks_;
    std::unordered_map<std::uint64_t, OrderLocation> order_index_;
};

} // namespace bookforge