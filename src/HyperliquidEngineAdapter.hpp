#pragma once

#include "ExternalOrderEvent.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

namespace bookforge {

enum class Side { Buy, Sell };

struct InternalAddOrder {
    std::uint64_t syntheticOrderId;
    Side side;
    double price;
    double size;
};

struct ReplayStats {
    std::size_t newCount{0};
    std::size_t cancelCount{0};
    std::size_t fillCount{0};
    std::size_t rejectCount{0};
    std::size_t triggerCount{0};
    std::size_t otherCount{0};
    std::size_t submittedToBook{0};
};

template <typename MatchingEngineT> class HyperliquidEngineAdapter {
  public:
    explicit HyperliquidEngineAdapter(MatchingEngineT &engine) : engine_(engine) {}

    void on_event(const ExternalOrderEvent &ev) {
        switch (ev.eventType) {
        case EventType::New:
            ++stats_.newCount;
            submit_new_order(ev);
            break;
        case EventType::Cancel:
            ++stats_.cancelCount;
            break;
        case EventType::Fill:
            ++stats_.fillCount;
            break;
        case EventType::Reject:
            ++stats_.rejectCount;
            break;
        case EventType::Trigger:
            ++stats_.triggerCount;
            break;
        case EventType::Other:
            ++stats_.otherCount;
            break;
        }
    }

    const ReplayStats &stats() const {
        return stats_;
    }

  private:
    void submit_new_order(const ExternalOrderEvent &ev) {
        InternalAddOrder order{.syntheticOrderId = nextSyntheticOrderId_++,
                               .side = ev.isAsk ? Side::Sell : Side::Buy,
                               .price = ev.price,
                               .size = ev.size};

        engine_.submit_external_order(order);
        ++stats_.submittedToBook;
    }

  private:
    MatchingEngineT &engine_;
    ReplayStats stats_{};
    std::uint64_t nextSyntheticOrderId_{1};
};

} // namespace bookforge