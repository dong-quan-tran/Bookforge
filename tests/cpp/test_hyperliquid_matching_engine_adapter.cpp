#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"

using namespace bookforge;

namespace {

ExternalOrderEvent MakeEvent(EventType event_type, bool is_ask, double price, double size,
                             int status_id = 1, const std::string &status_text = "open",
                             const std::string &external_order_id = {}) {
    ExternalOrderEvent event{};
    event.ts = std::chrono::nanoseconds{0};
    event.external_order_id = external_order_id;
    event.price = price;
    event.size = size;
    event.isAsk = is_ask;
    event.statusId = status_id;
    event.statusText = status_text;
    event.eventType = event_type;
    return event;
}

} // namespace

TEST(HyperliquidMatchingEngineAdapterTest, NewBidEventRestsInBook) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.01000));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1U);
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 1U);
    EXPECT_EQ(stats.generatedTrades, 0U);

    const auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);

    const auto bid_depth = engine.Book().GetBidDepth(1);
    ASSERT_EQ(bid_depth.size(), 1U);
    EXPECT_DOUBLE_EQ(bid_depth[0].first, 100.00);
    EXPECT_GT(bid_depth[0].second, 0U);
}

TEST(HyperliquidMatchingEngineAdapterTest, NewAskEventRestsInBook) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.02000));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1U);
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 1U);

    const auto best_ask = engine.Book().GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 100.50);

    const auto ask_depth = engine.Book().GetAskDepth(1);
    ASSERT_EQ(ask_depth.size(), 1U);
    EXPECT_DOUBLE_EQ(ask_depth[0].first, 100.50);
    EXPECT_GT(ask_depth[0].second, 0U);
}

TEST(HyperliquidMatchingEngineAdapterTest, CrossingNewEventsGenerateTrade) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.01000));
    adapter.OnEvent(MakeEvent(EventType::New, false, 101.00, 0.01000));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2U);
    EXPECT_EQ(stats.newCount, 2U);
    EXPECT_EQ(stats.submittedOrders, 2U);
    EXPECT_EQ(stats.generatedTrades, 1U);

    const std::vector<Trade> &trades = adapter.Trades();
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.50);
    EXPECT_GT(trades[0].quantity, 0U);
}

TEST(HyperliquidMatchingEngineAdapterTest, CancelsMappedRestingOrderByExternalId) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", "external-ask-1"));

    ASSERT_TRUE(engine.Book().GetBestAsk().has_value());

    adapter.OnEvent(
        MakeEvent(EventType::Cancel, true, 100.50, 0.01000, 2, "canceled", "external-ask-1"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2U);
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.cancelCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 1U);
    EXPECT_EQ(stats.canceledOrders, 1U);
    EXPECT_EQ(stats.ignoredEvents, 0U);
    EXPECT_EQ(adapter.Metrics().unsupported, 0U);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, DoesNotCancelUnknownExternalOrderId) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", "external-ask-1"));
    adapter.OnEvent(
        MakeEvent(EventType::Cancel, true, 100.50, 0.01000, 2, "canceled", "unknown-order"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.canceledOrders, 0U);
    EXPECT_EQ(stats.ignoredEvents, 1U);
    EXPECT_EQ(adapter.Metrics().unsupported, 1U);

    const auto best_ask = engine.Book().GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 100.50);
}

TEST(HyperliquidMatchingEngineAdapterTest, RepeatedCancelIsSafeNoOp) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", "external-ask-1"));
    adapter.OnEvent(
        MakeEvent(EventType::Cancel, true, 100.50, 0.01000, 2, "canceled", "external-ask-1"));
    adapter.OnEvent(
        MakeEvent(EventType::Cancel, true, 100.50, 0.01000, 2, "canceled", "external-ask-1"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.canceledOrders, 1U);
    EXPECT_EQ(stats.ignoredEvents, 1U);
    EXPECT_EQ(adapter.Metrics().unsupported, 1U);
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, CancelWithoutExternalIdIsIgnored) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Cancel, true, 100.50, 0.01000, 2, "canceled"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.cancelCount, 1U);
    EXPECT_EQ(stats.canceledOrders, 0U);
    EXPECT_EQ(stats.ignoredEvents, 1U);
    EXPECT_EQ(adapter.Metrics().unsupported, 1U);
}

TEST(HyperliquidMatchingEngineAdapterTest, FullyCrossedNewOrderIsNotRegisteredForCancel) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.00, 0.01000, 1, "open", "resting-ask"));
    adapter.OnEvent(MakeEvent(EventType::New, false, 101.00, 0.01000, 1, "open", "crossing-bid"));
    adapter.OnEvent(
        MakeEvent(EventType::Cancel, false, 101.00, 0.01000, 2, "canceled", "crossing-bid"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.generatedTrades, 1U);
    EXPECT_EQ(stats.canceledOrders, 0U);
    EXPECT_EQ(stats.ignoredEvents, 1U);
    EXPECT_EQ(adapter.Metrics().unsupported, 1U);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, RejectEventDoesNotTouchBook) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Reject, false, 100.00, 0.01000, 3, "perpMarginRejected"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1U);
    EXPECT_EQ(stats.rejectCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 0U);
    EXPECT_EQ(stats.ignoredEvents, 1U);
    EXPECT_EQ(stats.generatedTrades, 0U);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, FillEventIsCountedButIgnoredForNow) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Fill, false, 101.00, 0.01500, 5, "filled"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1U);
    EXPECT_EQ(stats.fillCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 0U);
    EXPECT_EQ(stats.ignoredEvents, 1U);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, TinyPositiveSizeRoundsUpToMinimumQuantity) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.000001));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 1U);

    const auto depth = engine.Book().GetBidDepth(1);
    ASSERT_EQ(depth.size(), 1U);
    EXPECT_EQ(depth[0].second, 1U);
}

TEST(HyperliquidMatchingEngineAdapterTest, ZeroSizeEventIsIgnoredAfterClassification) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.0));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1U);
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 0U);
    EXPECT_EQ(stats.ignoredEvents, 1U);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, MultipleEventsProduceSaneFinalBookState) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.01000));
    adapter.OnEvent(MakeEvent(EventType::New, false, 99.50, 0.02000));
    adapter.OnEvent(MakeEvent(EventType::New, true, 101.00, 0.01500));
    adapter.OnEvent(MakeEvent(EventType::Reject, true, 100.75, 0.01000, 3, "perpMarginRejected"));

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 4U);
    EXPECT_EQ(stats.newCount, 3U);
    EXPECT_EQ(stats.rejectCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 3U);
    EXPECT_EQ(stats.ignoredEvents, 1U);

    const auto best_bid = engine.Book().GetBestBid();
    const auto best_ask = engine.Book().GetBestAsk();

    ASSERT_TRUE(best_bid.has_value());
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);
    EXPECT_DOUBLE_EQ(*best_ask, 101.00);

    const auto spread = engine.Book().GetSpread();
    ASSERT_TRUE(spread.has_value());
    EXPECT_DOUBLE_EQ(*spread, 1.00);
}
