#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "IReplayAdapter.hpp"
#include "core/matching_engine.hpp"
#include "core/order.hpp"
#include "core/trade.hpp"

namespace bookforge {

struct ReplayStats {
    std::size_t totalEvents{0};
    std::size_t newCount{0};
    std::size_t cancelCount{0};
    std::size_t fillCount{0};
    std::size_t rejectCount{0};
    std::size_t triggerCount{0};
    std::size_t otherCount{0};

    std::size_t submittedOrders{0};
    std::size_t ignoredEvents{0};
    std::size_t generatedTrades{0};
};

class HyperliquidMatchingEngineAdapter final : public IReplayAdapter {
  public:
    explicit HyperliquidMatchingEngineAdapter(MatchingEngine &engine) : engine_(engine) {}

    void OnEvent(const ExternalOrderEvent &ev) override {
        ++stats_.totalEvents;

        switch (ev.eventType) {
        case EventType::New:
            ++stats_.newCount;
            ++metrics_.newEvents;
            SubmitNewOrder(ev);
            break;
        case EventType::Cancel:
            ++stats_.cancelCount;
            ++metrics_.cancelEvents;
            HandleCancel(ev);
            break;
        case EventType::Fill:
            ++stats_.fillCount;
            ++metrics_.fillEvents;
            HandleFill(ev);
            break;
        case EventType::Reject:
            ++stats_.rejectCount;
            ++metrics_.rejectEvents;
            HandleReject(ev);
            break;
        case EventType::Trigger:
            ++stats_.triggerCount;
            ++metrics_.triggerEvents;
            HandleTrigger(ev);
            break;
        case EventType::Other:
            ++stats_.otherCount;
            ++metrics_.otherEvents;
            HandleOther(ev);
            break;
        }
    }

    const AdapterMetrics &Metrics() const override {
        return metrics_;
    }

    const ReplayStats &Stats() const {
        return stats_;
    }

    const std::vector<Trade> &Trades() const {
        return trades_;
    }

  private:
    void SubmitNewOrder(const ExternalOrderEvent &ev) {
        Order order{};
        order.id = nextSyntheticOrderId_++;
        order.participant_id = 0;
        order.side = ev.isAsk ? Side::Sell : Side::Buy;
        order.price = ev.price;
        order.quantity = ToInternalQuantity(ev.size);
        order.timestamp = nextSyntheticTimestamp_++;
        order.stp = SelfTradePrevention::None;

        if (order.quantity == 0) {
            ++stats_.ignoredEvents;
            ++metrics_.ignored;
            return;
        }

        MatchResult result = engine_.MatchLimitOrder(order);

        ++stats_.submittedOrders;
        ++metrics_.submitted;
        stats_.generatedTrades += result.trades.size();

        for (const auto &trade : result.trades) {
            trades_.push_back(trade);
        }
    }

    void HandleCancel(const ExternalOrderEvent &) {
        ++stats_.ignoredEvents;
        ++metrics_.unsupported;
    }

    void HandleFill(const ExternalOrderEvent &) {
        ++stats_.ignoredEvents;
        ++metrics_.unsupported;
    }

    void HandleReject(const ExternalOrderEvent &) {
        ++stats_.ignoredEvents;
        ++metrics_.rejected;
    }

    void HandleTrigger(const ExternalOrderEvent &) {
        ++stats_.ignoredEvents;
        ++metrics_.ignored;
    }

    void HandleOther(const ExternalOrderEvent &) {
        ++stats_.ignoredEvents;
        ++metrics_.ignored;
    }

    static std::uint32_t ToInternalQuantity(double size) {
        constexpr double scale = 100000.0;
        const double scaled = size * scale;

        if (scaled <= 0.0) {
            return 0;
        }

        const auto qty = static_cast<std::uint64_t>(scaled);
        if (qty == 0) {
            return 1;
        }
        if (qty > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
            return std::numeric_limits<std::uint32_t>::max();
        }
        return static_cast<std::uint32_t>(qty);
    }

  private:
    MatchingEngine &engine_;
    ReplayStats stats_{};
    AdapterMetrics metrics_{};
    std::vector<Trade> trades_{};

    std::uint64_t nextSyntheticOrderId_{1};
    std::uint64_t nextSyntheticTimestamp_{1};
};

} // namespace bookforge