#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
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
    std::size_t canceledOrders{0};
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

    void OnEvent(const ExternalOrderEvent &event) override {
        ++stats_.totalEvents;

        switch (event.eventType) {
        case EventType::New:
            ++stats_.newCount;
            ++metrics_.newEvents;
            SubmitExternalNewOrder(event);
            break;
        case EventType::Cancel:
            ++stats_.cancelCount;
            ++metrics_.cancelEvents;
            HandleCancel(event);
            break;
        case EventType::Fill:
            ++stats_.fillCount;
            ++metrics_.fillEvents;
            HandleFill(event);
            break;
        case EventType::Reject:
            ++stats_.rejectCount;
            ++metrics_.rejectEvents;
            HandleReject(event);
            break;
        case EventType::Trigger:
            ++stats_.triggerCount;
            ++metrics_.triggerEvents;
            HandleTrigger(event);
            break;
        case EventType::Other:
            ++stats_.otherCount;
            ++metrics_.otherEvents;
            HandleOther(event);
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

        for (const Trade &trade : result.trades) {
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
    struct SubmittedOrder {
        std::uint64_t internal_order_id{0};
        MatchResult match_result;
    };

    void SubmitExternalNewOrder(const ExternalOrderEvent &event) {
        const Side side = event.isAsk ? Side::Sell : Side::Buy;
        const std::uint32_t quantity = ToInternalQuantity(event.size);
        const SubmittedOrder submitted_order = SubmitOrderWithId(side, event.price, quantity);

        if (event.external_order_id.empty() || submitted_order.internal_order_id == 0 ||
            submitted_order.match_result.remaining_quantity != 0) {
            return;
        }

        external_to_internal_order_ids_[event.external_order_id] =
            submitted_order.internal_order_id;
    }

    MatchResult SubmitOrder(Side side, double price, std::uint32_t quantity) {
        return SubmitOrderWithId(side, price, quantity).match_result;
    }

    SubmittedOrder SubmitOrderWithId(Side side, double price, std::uint32_t quantity) {
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

        for (const Trade &trade : result.trades) {
            trades_.push_back(trade);
        }

        return SubmittedOrder{order.id, std::move(result)};
    }

    void HandleCancel(const ExternalOrderEvent &event) {
        if (event.external_order_id.empty()) {
            ++stats_.ignoredEvents;
            ++metrics_.unsupported;
            return;
        }

        const auto mapping_it = external_to_internal_order_ids_.find(event.external_order_id);
        if (mapping_it == external_to_internal_order_ids_.end()) {
            ++stats_.ignoredEvents;
            ++metrics_.unsupported;
            return;
        }

        const std::uint64_t internal_order_id = mapping_it->second;
        external_to_internal_order_ids_.erase(mapping_it);

        if (!engine_.CancelOrder(internal_order_id)) {
            ++stats_.ignoredEvents;
            ++metrics_.unsupported;
            return;
        }

        ++stats_.canceledOrders;
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

        const auto quantity = static_cast<std::uint64_t>(scaled);
        if (quantity == 0) {
            return 1;
        }

        if (quantity > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
            return std::numeric_limits<std::uint32_t>::max();
        }

        return static_cast<std::uint32_t>(quantity);
    }

  private:
    MatchingEngine &engine_;
    InjectedOrderFillHandler injected_order_fill_handler_;
    ReplayStats stats_{};
    AdapterMetrics metrics_{};
    std::vector<Trade> trades_{};
    std::unordered_map<std::string, std::uint64_t> external_to_internal_order_ids_{};

    std::uint64_t nextSyntheticOrderId_{1};
    std::uint64_t nextSyntheticTimestamp_{1};
};

} // namespace bookforge
