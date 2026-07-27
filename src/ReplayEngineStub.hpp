#pragma once

#include "HyperliquidEngineAdapter.hpp"

#include <vector>

namespace bookforge {

class ReplayEngineStub {
  public:
    void submit_external_order(const InternalAddOrder &order) {
        submitted_.push_back(order);
    }

    const std::vector<InternalAddOrder> &submitted_orders() const {
        return submitted_;
    }

  private:
    std::vector<InternalAddOrder> submitted_;
};

} // namespace bookforge