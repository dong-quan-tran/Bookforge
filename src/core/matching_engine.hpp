#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "order_book.hpp"
#include "trade.hpp"

namespace bookforge {

struct MatchResult {
    std::vector<Trade> trades;
    std::uint32_t remaining_quantity{0};
};

struct TopOfBookSnapshot {
    std::optional<double> best_bid;
    std::optional<double> best_ask;
    std::optional<double> mid_price;
    std::optional<double> spread;
    std::optional<std::uint32_t> best_bid_volume;
    std::optional<std::uint32_t> best_ask_volume;
};

enum class EngineEventType : std::uint8_t { Accepted, Rested, Canceled, TradeExecuted, Rejected };

struct EngineEventLogEntry {
    EngineEventType type;
    std::uint64_t order_id{0};
    std::uint64_t related_order_id{0};
    double price{0.0};
    std::uint32_t quantity{0};
    std::uint64_t timestamp{0};
};

class MatchingEngine {
  public:
    MatchingEngine() = default;

    OrderBook &Book() {
        return book_;
    }
    const OrderBook &Book() const {
        return book_;
    }

    MatchResult MatchLimitOrder(const Order &order);
    bool CancelOrder(std::uint64_t order_id);

    [[nodiscard]] TopOfBookSnapshot CaptureTopOfBook() const;
    [[nodiscard]] const std::vector<EngineEventLogEntry> &EventLog() const {
        return event_log_;
    }

  private:
    void LogEvent(EngineEventType type, std::uint64_t order_id, std::uint64_t related_order_id,
                  double price, std::uint32_t quantity, std::uint64_t timestamp);

  private:
    OrderBook book_;
    std::vector<EngineEventLogEntry> event_log_{};
};

} // namespace bookforge