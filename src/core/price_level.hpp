#pragma once

#include <cstdint>
#include <list>
#include <optional>

#include "order.hpp"

namespace bookforge {

class PriceLevel {
  public:
    using OrderList = std::list<Order>;
    using Iterator = OrderList::iterator;

    explicit PriceLevel(double price = 0.0);

    Iterator AddOrder(const Order &order);
    void RemoveOrder(Iterator it);
    std::optional<Order> PopFrontOrder();
    bool ReduceFrontOrder(std::uint32_t quantity);
    bool ReduceOrderQuantity(Iterator it, std::uint32_t new_quantity);

    [[nodiscard]] std::optional<Order> FrontOrder() const;
    [[nodiscard]] double GetPrice() const;
    [[nodiscard]] std::uint32_t TotalVolume() const;
    [[nodiscard]] bool Empty() const;
    [[nodiscard]] std::size_t OrderCount() const;

  private:
    double price_;
    OrderList orders_;
};

} // namespace bookforge