#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "IReplayAdapter.hpp"
#include "core/matching_engine.hpp"
#include "core/order.hpp"
#include "core/trade.hpp"
#include "replay/InjectedOrder.hpp"

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

    std::size_t injectedOrders{0};
};

class HyperliquidMatchingEngineAdapter final : public IReplayAdapter {
  public:
    using InjectedOrderFillHandler = std::function<void(const InjectedOrder &, const Trade &)>;

    explicit HyperliquidMatchingEngineAdapter(
        MatchingEngine &engine, InjectedOrderFillHandler injected_order_fill_handler = {})
        : engine_(engine), injected_order_fill_handler_(std::move(injected_order_fill_handler)) {}

    void OnEvent(const ExternalOrderEvent &ev) override {
        ++stats_.totalEvents;

        switch (ev.eventType) {
        case EventType::New:
            ++stats_.newCount;
            ++metrics_.newEvents;
            SubmitExternalNewOrder(ev);
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

    void OnInjectedOrder(const InjectedOrder &order) override {
        ++stats_.injectedOrders;

        const MatchResult result =
            SubmitOrder(order.is_buy ? Side::Buy : Side::Sell, order.price, order.quantity);

        if (!injected_order_fill_handler_) {
            return;
        }

        for (const auto &trade : result.trades) {
            injected_order_fill_handler_(order, trade);
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
    void SubmitExternalNewOrder(const ExternalOrderEvent &ev) {
        const Side side = ev.isAsk ? Side::Sell : Side::Buy;
        const std::uint32_t quantity = ToInternalQuantity(ev.size);

        (void)SubmitOrder(side, ev.price, quantity);
    }

    MatchResult SubmitOrder(Side side, double price, std::uint32_t quantity) {
        if (quantity == 0) {
            ++stats_.ignoredEvents;
            ++metrics_.ignored;
            return {};
        }

        Order order{};
        order.id = nextSyntheticOrderId_++;
        order.participant_id = 0;
        order.side = side;
        order.price = price;
        order.quantity = quantity;
        order.timestamp = nextSyntheticTimestamp_++;
        order.stp = SelfTradePrevention::None;

        MatchResult result = engine_.MatchLimitOrder(order);

        ++stats_.submittedOrders;
        ++metrics_.submitted;
        stats_.generatedTrades += result.trades.size();

        for (const auto &trade : result.trades) {
            trades_.push_back(trade);
        }

        return result;
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
    InjectedOrderFillHandler injected_order_fill_handler_;
    ReplayStats stats_{};
    AdapterMetrics metrics_{};
    std::vector<Trade> trades_{};

    std::uint64_t nextSyntheticOrderId_{1};
    std::uint64_t nextSyntheticTimestamp_{1};
};

} // namespace bookforge