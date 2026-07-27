#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

using namespace bookforge;

namespace {

ExternalOrderEvent MakeEvent(EventType type, bool isAsk, double price, double size,
                             int statusId = 1, const std::string &statusText = "open",
                             std::int64_t ts_ns = 0) {
    ExternalOrderEvent ev{};
    ev.ts = std::chrono::nanoseconds{ts_ns};
    ev.price = price;
    ev.size = size;
    ev.isAsk = isAsk;
    ev.statusId = statusId;
    ev.statusText = statusText;
    ev.eventType = type;
    return ev;
}

} // namespace

TEST(ReplayRunnerTest, ProcessesOnlyBoundedPrefixWhenMaxEventsSet) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::Reject, false, 101.00, 0.02000, 3, "perpMarginRejected", 2),
        MakeEvent(EventType::New, true, 102.00, 0.03000, 1, "open", 3),
    };

    ReplayConfig config;
    config.max_events = 2;
    config.start_offset = 0;
    config.log_every_n = 0;
    config.log_summary = false;
    config.log_errors = false;
    config.strict_mode = false;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2u);
    EXPECT_EQ(stats.newCount, 1u);
    EXPECT_EQ(stats.rejectCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 1u);
    EXPECT_EQ(stats.ignoredEvents, 1u);

    auto bestBid = engine.Book().GetBestBid();
    ASSERT_TRUE(bestBid.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 100.00);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(ReplayRunnerTest, AppliesStartOffsetBeforeMaxEvents) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 2),
        MakeEvent(EventType::Reject, false, 101.00, 0.02000, 3, "perpMarginRejected", 3),
        MakeEvent(EventType::New, false, 99.75, 0.02000, 1, "open", 4),
    };

    ReplayConfig config;
    config.start_offset = 1;
    config.max_events = 2;
    config.log_every_n = 0;
    config.log_summary = false;
    config.log_errors = false;
    config.strict_mode = false;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2u);
    EXPECT_EQ(stats.newCount, 1u);
    EXPECT_EQ(stats.rejectCount, 1u);
    EXPECT_EQ(stats.submittedOrders, 1u);
    EXPECT_EQ(stats.ignoredEvents, 1u);

    auto bestAsk = engine.Book().GetBestAsk();
    ASSERT_TRUE(bestAsk.has_value());
    EXPECT_DOUBLE_EQ(*bestAsk, 100.50);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
}

TEST(ReplayRunnerTest, PreservesInputOrderingExactly) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 10),
        MakeEvent(EventType::New, false, 101.00, 0.01000, 1, "open", 20),
        MakeEvent(EventType::New, false, 99.50, 0.02000, 1, "open", 30),
    };

    ReplayConfig config;
    config.start_offset = 0;
    config.max_events = 0;
    config.log_every_n = 0;
    config.log_summary = false;
    config.log_errors = false;
    config.strict_mode = false;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 3u);
    EXPECT_EQ(stats.newCount, 3u);
    EXPECT_EQ(stats.generatedTrades, 1u);

    const auto &trades = adapter.Trades();
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.50);
    EXPECT_EQ(trades[0].side, Side::Buy);

    auto bestBid = engine.Book().GetBestBid();
    ASSERT_TRUE(bestBid.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 99.50);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(ReplayRunnerTest, OffsetPastEndProcessesNothing) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 2),
    };

    ReplayConfig config;
    config.start_offset = 10;
    config.max_events = 5;
    config.log_every_n = 0;
    config.log_summary = false;
    config.log_errors = false;
    config.strict_mode = false;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 0u);
    EXPECT_EQ(stats.newCount, 0u);
    EXPECT_EQ(stats.submittedOrders, 0u);
    EXPECT_EQ(stats.generatedTrades, 0u);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}