#include "replay/SymbolReplayFilter.hpp"

#include <chrono>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

ExternalOrderEvent MakeEvent(const std::string &symbol, double price) {
    ExternalOrderEvent event{};
    event.ts = std::chrono::nanoseconds{1};
    event.symbol = symbol;
    event.price = price;
    event.size = 0.01;
    event.isAsk = false;
    event.statusId = 1;
    event.statusText = "open";
    event.eventType = EventType::New;
    return event;
}

TEST(SymbolReplayFilterTest, ReturnsAllEventsWhenNoSymbolIsSelected) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent("BTC", 100.0),
        MakeEvent("ETH", 200.0),
        MakeEvent("", 300.0),
    };

    const auto filtered_events = FilterReplayEventsBySymbol(events, "", "BTCUSDT.P");

    ASSERT_EQ(filtered_events.size(), 3U);
    EXPECT_EQ(filtered_events[0].symbol, "BTC");
    EXPECT_EQ(filtered_events[1].symbol, "ETH");
    EXPECT_TRUE(filtered_events[2].symbol.empty());
}

TEST(SymbolReplayFilterTest, KeepsOnlyExplicitMatchingSymbolEvents) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent("BTC", 100.0),
        MakeEvent("ETH", 200.0),
        MakeEvent("BTC", 101.0),
    };

    const auto filtered_events = FilterReplayEventsBySymbol(events, "BTC", "BTCUSDT.P");

    ASSERT_EQ(filtered_events.size(), 2U);
    EXPECT_EQ(filtered_events[0].symbol, "BTC");
    EXPECT_DOUBLE_EQ(filtered_events[0].price, 100.0);
    EXPECT_EQ(filtered_events[1].symbol, "BTC");
    EXPECT_DOUBLE_EQ(filtered_events[1].price, 101.0);
}

TEST(SymbolReplayFilterTest, TreatsSymbolLessEventsAsFallbackSymbol) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent("", 100.0),
        MakeEvent("BTCUSDT.P", 101.0),
        MakeEvent("ETHUSDT.P", 102.0),
    };

    const auto filtered_events = FilterReplayEventsBySymbol(events, "BTCUSDT.P", "BTCUSDT.P");

    ASSERT_EQ(filtered_events.size(), 2U);
    EXPECT_TRUE(filtered_events[0].symbol.empty());
    EXPECT_EQ(filtered_events[1].symbol, "BTCUSDT.P");
}

TEST(SymbolReplayFilterTest, ExcludesSymbolLessEventsForDifferentSelectedSymbol) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent("", 100.0),
        MakeEvent("BTCUSDT.P", 101.0),
        MakeEvent("ETHUSDT.P", 102.0),
    };

    const auto filtered_events = FilterReplayEventsBySymbol(events, "ETHUSDT.P", "BTCUSDT.P");

    ASSERT_EQ(filtered_events.size(), 1U);
    EXPECT_EQ(filtered_events[0].symbol, "ETHUSDT.P");
}

TEST(SymbolReplayFilterTest, ReturnsNoEventsWhenSelectedSymbolIsAbsent) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent("BTC", 100.0),
        MakeEvent("ETH", 200.0),
    };

    const auto filtered_events = FilterReplayEventsBySymbol(events, "SOL", "BTCUSDT.P");

    EXPECT_TRUE(filtered_events.empty());
}

} // namespace
} // namespace bookforge
