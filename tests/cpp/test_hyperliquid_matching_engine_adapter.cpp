#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"

using namespace bookforge;

namespace {

ExternalOrderEvent MakeEvent(EventType eventType, bool isAsk, double price, double size,
                             int statusId = 1, const std::string &statusText = "open") {
    ExternalOrderEvent ev{};
    ev.ts = std::chrono::nanoseconds{0};
    ev.price = price;
    ev.size = size;
    ev.isAsk = isAsk;
    ev.statusId = statusId;
    ev.statusText = statusText;
    ev.eventType = eventType;
    return ev;
}

} // namespace

TEST(HyperliquidMatchingEngineAdapterTest, NewBidEventRestsInBook) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.01000));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1u);
    EXPECT_EQ(stats.newCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 1u);
    EXPECT_EQ(stats.generatedTrades, 0u);

    auto bestBid = engine.Book().GetBestBid();
    ASSERT_TRUE(bestBid.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 100.00);

    auto bidDepth = engine.Book().GetBidDepth(1);
    ASSERT_EQ(bidDepth.size(), 1u);
    EXPECT_DOUBLE_EQ(bidDepth[0].first, 100.00);
    EXPECT_GT(bidDepth[0].second, 0u);
}

TEST(HyperliquidMatchingEngineAdapterTest, NewAskEventRestsInBook) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.02000));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1u);
    EXPECT_EQ(stats.newCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 1u);

    auto bestAsk = engine.Book().GetBestAsk();
    ASSERT_TRUE(bestAsk.has_value());
    EXPECT_DOUBLE_EQ(*bestAsk, 100.50);

    auto askDepth = engine.Book().GetAskDepth(1);
    ASSERT_EQ(askDepth.size(), 1u);
    EXPECT_DOUBLE_EQ(askDepth[0].first, 100.50);
    EXPECT_GT(askDepth[0].second, 0u);
}

TEST(HyperliquidMatchingEngineAdapterTest, CrossingNewEventsGenerateTrade) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, true, 100.50, 0.01000));
    adapter.OnEvent(MakeEvent(EventType::New, false, 101.00, 0.01000));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2u);
    EXPECT_EQ(stats.newCount, 2u);
    EXPECT_EQ(stats.submittedOrders, 2u);
    EXPECT_EQ(stats.generatedTrades, 1u);

    const auto &trades = adapter.Trades();
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.50);
    EXPECT_GT(trades[0].quantity, 0u);
}

TEST(HyperliquidMatchingEngineAdapterTest, RejectEventDoesNotTouchBook) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Reject, false, 100.00, 0.01000, 3, "perpMarginRejected"));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1u);
    EXPECT_EQ(stats.rejectCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 0u);
    EXPECT_EQ(stats.ignoredEvents, 1u);
    EXPECT_EQ(stats.generatedTrades, 0u);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, CancelEventIsCountedButIgnoredForNow) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Cancel, true, 100.50, 0.01000, 2, "canceled"));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1u);
    EXPECT_EQ(stats.cancelCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 0u);
    EXPECT_EQ(stats.ignoredEvents, 1u);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, FillEventIsCountedButIgnoredForNow) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Fill, false, 101.00, 0.01500, 5, "filled"));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1u);
    EXPECT_EQ(stats.fillCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 0u);
    EXPECT_EQ(stats.ignoredEvents, 1u);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(HyperliquidMatchingEngineAdapterTest, TinyPositiveSizeRoundsUpToMinimumQuantity) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.000001));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.newCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 1u);

    auto depth = engine.Book().GetBidDepth(1);
    ASSERT_EQ(depth.size(), 1u);
    EXPECT_EQ(depth[0].second, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, ZeroSizeEventIsIgnoredAfterClassification) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, false, 100.00, 0.0));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 1u);
    EXPECT_EQ(stats.newCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 0u);
    EXPECT_EQ(stats.ignoredEvents, 1u);

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

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 4u);
    EXPECT_EQ(stats.newCount, 3u);
    EXPECT_EQ(stats.rejectCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 3u);
    EXPECT_EQ(stats.ignoredEvents, 1u);

    auto bestBid = engine.Book().GetBestBid();
    auto bestAsk = engine.Book().GetBestAsk();

    ASSERT_TRUE(bestBid.has_value());
    ASSERT_TRUE(bestAsk.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 100.00);
    EXPECT_DOUBLE_EQ(*bestAsk, 101.00);

    auto spread = engine.Book().GetSpread();
    ASSERT_TRUE(spread.has_value());
    EXPECT_DOUBLE_EQ(*spread, 1.00);
}