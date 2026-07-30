#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "replay/InjectedOrder.hpp"

namespace bookforge {

class InjectedOrderSchedule {
  public:
    void Add(const InjectedOrder &order) {
        schedule_[order.trigger_event_index].push_back(order);
    }

    [[nodiscard]] const std::vector<InjectedOrder> *Find(std::size_t event_index) const {
        const auto it = schedule_.find(event_index);
        if (it == schedule_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] bool Empty() const {
        return schedule_.empty();
    }

  private:
    std::unordered_map<std::size_t, std::vector<InjectedOrder>> schedule_;
};

} // namespace bookforge