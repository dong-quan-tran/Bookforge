#include "matching_engine.hpp"

namespace bookforge {

namespace {

bool IsCrossingLimit(const Order &order, const std::optional<double> &best_bid,
                     const std::optional<double> &best_ask) {
    if (order.side == Side::Buy) {
        if (!best_ask.has_value()) {
            return false;
        }
        return order.price >= *best_ask;
    }

    if (!best_bid.has_value()) {
        return false;
    }
    return order.price <= *best_bid;
}

} // namespace

MatchResult MatchingEngine::MatchLimitOrder(const Order &incoming) {
    MatchResult result{};
    std::uint32_t remaining = incoming.quantity;

    LogEvent(EngineEventType::Accepted, incoming.id, 0, incoming.price, incoming.quantity,
             incoming.timestamp);

    while (remaining > 0) {
        auto best_bid = book_.GetBestBid();
        auto best_ask = book_.GetBestAsk();

        if (!IsCrossingLimit(incoming, best_bid, best_ask)) {
            break;
        }

        if (incoming.side == Side::Buy) {
            auto maker = book_.PeekBestAskOrder();
            if (!maker.has_value()) {
                break;
            }

            if (incoming.participant_id != 0 && incoming.participant_id == maker->participant_id) {
                if (incoming.stp == SelfTradePrevention::CancelNewest) {
                    result.remaining_quantity = incoming.quantity;
                    LogEvent(EngineEventType::Rejected, incoming.id, maker->id, incoming.price,
                             incoming.quantity, incoming.timestamp);
                    return result;
                }

                if (incoming.stp == SelfTradePrevention::CancelOldest) {
                    bool canceled = book_.CancelOrder(maker->id);
                    if (!canceled) {
                        break;
                    }
                    LogEvent(EngineEventType::Canceled, maker->id, incoming.id, maker->price,
                             maker->quantity, incoming.timestamp);
                    continue;
                }
            }

            const std::uint32_t executed_qty =
                remaining <= maker->quantity ? remaining : maker->quantity;

            result.trades.push_back(Trade{incoming.id, maker->id, Side::Buy, maker->price,
                                          executed_qty, incoming.timestamp});

            LogEvent(EngineEventType::TradeExecuted, incoming.id, maker->id, maker->price,
                     executed_qty, incoming.timestamp);

            bool ok = book_.ExecuteTopOrder(Side::Sell, maker->price, executed_qty);
            if (!ok) {
                break;
            }

            remaining -= executed_qty;
        } else {
            auto maker = book_.PeekBestBidOrder();
            if (!maker.has_value()) {
                break;
            }

            if (incoming.participant_id != 0 && incoming.participant_id == maker->participant_id) {
                if (incoming.stp == SelfTradePrevention::CancelNewest) {
                    result.remaining_quantity = incoming.quantity;
                    LogEvent(EngineEventType::Rejected, incoming.id, maker->id, incoming.price,
                             incoming.quantity, incoming.timestamp);
                    return result;
                }

                if (incoming.stp == SelfTradePrevention::CancelOldest) {
                    bool canceled = book_.CancelOrder(maker->id);
                    if (!canceled) {
                        break;
                    }
                    LogEvent(EngineEventType::Canceled, maker->id, incoming.id, maker->price,
                             maker->quantity, incoming.timestamp);
                    continue;
                }
            }

            const std::uint32_t executed_qty =
                remaining <= maker->quantity ? remaining : maker->quantity;

            result.trades.push_back(Trade{incoming.id, maker->id, Side::Sell, maker->price,
                                          executed_qty, incoming.timestamp});

            LogEvent(EngineEventType::TradeExecuted, incoming.id, maker->id, maker->price,
                     executed_qty, incoming.timestamp);

            bool ok = book_.ExecuteTopOrder(Side::Buy, maker->price, executed_qty);
            if (!ok) {
                break;
            }

            remaining -= executed_qty;
        }
    }

    if (remaining > 0) {
        Order resting = incoming;
        resting.quantity = remaining;

        if (book_.AddOrder(resting)) {
            LogEvent(EngineEventType::Rested, resting.id, 0, resting.price, resting.quantity,
                     resting.timestamp);
            result.remaining_quantity = 0;
        } else {
            LogEvent(EngineEventType::Rejected, resting.id, 0, resting.price, resting.quantity,
                     resting.timestamp);
            result.remaining_quantity = remaining;
        }
    } else {
        result.remaining_quantity = 0;
    }

    return result;
}

bool MatchingEngine::CancelOrder(std::uint64_t order_id) {
    auto best_bid_before = book_.GetBestBid();
    auto best_ask_before = book_.GetBestAsk();

    auto bid_order = book_.PeekBestBidOrder();
    auto ask_order = book_.PeekBestAskOrder();

    bool ok = book_.CancelOrder(order_id);
    if (!ok) {
        return false;
    }

    double price = 0.0;
    std::uint32_t quantity = 0;

    if (bid_order.has_value() && bid_order->id == order_id) {
        price = bid_order->price;
        quantity = bid_order->quantity;
    } else if (ask_order.has_value() && ask_order->id == order_id) {
        price = ask_order->price;
        quantity = ask_order->quantity;
    } else {
        if (best_bid_before.has_value()) {
            price = *best_bid_before;
        } else if (best_ask_before.has_value()) {
            price = *best_ask_before;
        }
    }

    LogEvent(EngineEventType::Canceled, order_id, 0, price, quantity, 0);
    return true;
}

TopOfBookSnapshot MatchingEngine::CaptureTopOfBook() const {
    TopOfBookSnapshot s{};
    s.best_bid = book_.GetBestBid();
    s.best_ask = book_.GetBestAsk();
    s.mid_price = book_.GetMidPrice();
    s.spread = book_.GetSpread();

    if (s.best_bid.has_value()) {
        s.best_bid_volume = book_.GetLevelVolume(Side::Buy, *s.best_bid);
    }

    if (s.best_ask.has_value()) {
        s.best_ask_volume = book_.GetLevelVolume(Side::Sell, *s.best_ask);
    }

    return s;
}

void MatchingEngine::LogEvent(EngineEventType type, std::uint64_t order_id,
                              std::uint64_t related_order_id, double price, std::uint32_t quantity,
                              std::uint64_t timestamp) {
    event_log_.push_back(
        EngineEventLogEntry{type, order_id, related_order_id, price, quantity, timestamp});
}

} // namespace bookforge