#include "replay/MultiSymbolReplayAdapter.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

ExternalOrderEvent MakeEvent(const std::string &symbol, EventType event_type, bool is_ask,
                             double price, double size, std::int64_t timestamp_ns = 0) {
    ExternalOrderEvent event{};
    event.ts = std::chrono::nanoseconds{timestamp_ns};
    event.symbol = symbol;
    event.price = price;
    event.size = size;
    event.isAsk = is_ask;
    event.statusId = 1;
    event.statusText = "open";
    event.eventType = event_type;
    return event;
}

InjectedOrder MakeInjectedOrder(const std::string &symbol, bool is_buy, double price,
                                std::uint32_t quantity) {
    InjectedOrder order{};
    order.order_id = "injected-order";
    order.participant_id = "strategy";
    order.symbol = symbol;
    order.is_buy = is_buy;
    order.price = price;
    order.quantity = quantity;
    return order;
}

TEST(MultiSymbolReplayAdapterTest, CreatesIndependentBooksForDifferentSymbols) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnEvent(MakeEvent("BTC", EventType::New, false, 100.0, 0.01));
    adapter.OnEvent(MakeEvent("ETH", EventType::New, true, 100.0, 0.01));

    ASSERT_EQ(adapter.SymbolCount(), 2U);

    const MatchingEngine *btc_engine = adapter.FindEngine("BTC");
    const MatchingEngine *eth_engine = adapter.FindEngine("ETH");

    ASSERT_NE(btc_engine, nullptr);
    ASSERT_NE(eth_engine, nullptr);

    const auto btc_best_bid = btc_engine->Book().GetBestBid();
    ASSERT_TRUE(btc_best_bid.has_value());
    EXPECT_DOUBLE_EQ(*btc_best_bid, 100.0);
    EXPECT_FALSE(btc_engine->Book().GetBestAsk().has_value());

    const auto eth_best_ask = eth_engine->Book().GetBestAsk();
    ASSERT_TRUE(eth_best_ask.has_value());
    EXPECT_DOUBLE_EQ(*eth_best_ask, 100.0);
    EXPECT_FALSE(eth_engine->Book().GetBestBid().has_value());

    const auto *btc_adapter = adapter.FindAdapter("BTC");
    const auto *eth_adapter = adapter.FindAdapter("ETH");

    ASSERT_NE(btc_adapter, nullptr);
    ASSERT_NE(eth_adapter, nullptr);
    EXPECT_TRUE(btc_adapter->Trades().empty());
    EXPECT_TRUE(eth_adapter->Trades().empty());
}

TEST(MultiSymbolReplayAdapterTest, CrossingOrderTradesOnlyWithinItsOwnSymbol) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnEvent(MakeEvent("BTC", EventType::New, true, 100.0, 0.01));
    adapter.OnEvent(MakeEvent("ETH", EventType::New, true, 100.0, 0.01));
    adapter.OnEvent(MakeEvent("BTC", EventType::New, false, 101.0, 0.01));

    const auto *btc_adapter = adapter.FindAdapter("BTC");
    const auto *eth_adapter = adapter.FindAdapter("ETH");

    ASSERT_NE(btc_adapter, nullptr);
    ASSERT_NE(eth_adapter, nullptr);

    ASSERT_EQ(btc_adapter->Trades().size(), 1U);
    EXPECT_DOUBLE_EQ(btc_adapter->Trades()[0].price, 100.0);
    EXPECT_EQ(btc_adapter->Trades()[0].side, Side::Buy);

    EXPECT_TRUE(eth_adapter->Trades().empty());

    const MatchingEngine *eth_engine = adapter.FindEngine("ETH");
    ASSERT_NE(eth_engine, nullptr);

    const auto eth_best_ask = eth_engine->Book().GetBestAsk();
    ASSERT_TRUE(eth_best_ask.has_value());
    EXPECT_DOUBLE_EQ(*eth_best_ask, 100.0);
}

TEST(MultiSymbolReplayAdapterTest, RoutesSymbolLessEventsToConfiguredFallbackSymbol) {
    MultiSymbolReplayAdapter adapter("BTC");

    adapter.OnEvent(MakeEvent("", EventType::New, false, 100.0, 0.01));

    ASSERT_EQ(adapter.SymbolCount(), 1U);
    EXPECT_NE(adapter.FindEngine("BTC"), nullptr);
    EXPECT_EQ(adapter.FindEngine(""), nullptr);

    const MatchingEngine *btc_engine = adapter.FindEngine("BTC");
    ASSERT_NE(btc_engine, nullptr);

    const auto best_bid = btc_engine->Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.0);
}

TEST(MultiSymbolReplayAdapterTest, IgnoresSymbolLessEventsWithoutFallbackSymbol) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnEvent(MakeEvent("", EventType::New, false, 100.0, 0.01));

    EXPECT_EQ(adapter.SymbolCount(), 0U);
    EXPECT_EQ(adapter.Metrics().ignored, 1U);
    EXPECT_EQ(adapter.Metrics().submitted, 0U);
}

TEST(MultiSymbolReplayAdapterTest, RoutesInjectedOrderToItsSelectedSymbol) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnEvent(MakeEvent("BTC", EventType::New, true, 100.0, 0.01));
    adapter.OnEvent(MakeEvent("ETH", EventType::New, true, 90.0, 0.01));

    adapter.OnInjectedOrder(MakeInjectedOrder("BTC", true, 101.0, 1000));

    const HyperliquidMatchingEngineAdapter *btc_adapter = adapter.FindAdapter("BTC");
    const HyperliquidMatchingEngineAdapter *eth_adapter = adapter.FindAdapter("ETH");

    ASSERT_NE(btc_adapter, nullptr);
    ASSERT_NE(eth_adapter, nullptr);

    ASSERT_EQ(btc_adapter->Trades().size(), 1U);
    EXPECT_DOUBLE_EQ(btc_adapter->Trades()[0].price, 100.0);
    EXPECT_EQ(btc_adapter->Trades()[0].side, Side::Buy);

    EXPECT_TRUE(eth_adapter->Trades().empty());

    const MatchingEngine *eth_engine = adapter.FindEngine("ETH");
    ASSERT_NE(eth_engine, nullptr);

    const auto eth_best_ask = eth_engine->Book().GetBestAsk();
    ASSERT_TRUE(eth_best_ask.has_value());
    EXPECT_DOUBLE_EQ(*eth_best_ask, 90.0);

    EXPECT_EQ(adapter.Metrics().submitted, 3U);
    EXPECT_EQ(adapter.Metrics().unsupported, 0U);
}

TEST(MultiSymbolReplayAdapterTest, RoutesSymbolLessInjectedOrderToFallbackSymbol) {
    MultiSymbolReplayAdapter adapter("BTC");

    adapter.OnEvent(MakeEvent("BTC", EventType::New, true, 100.0, 0.01));
    adapter.OnInjectedOrder(MakeInjectedOrder("", true, 101.0, 1000));

    const HyperliquidMatchingEngineAdapter *btc_adapter = adapter.FindAdapter("BTC");
    ASSERT_NE(btc_adapter, nullptr);

    ASSERT_EQ(btc_adapter->Trades().size(), 1U);
    EXPECT_DOUBLE_EQ(btc_adapter->Trades()[0].price, 100.0);
    EXPECT_EQ(adapter.SymbolCount(), 1U);
    EXPECT_EQ(adapter.Metrics().submitted, 2U);
}

TEST(MultiSymbolReplayAdapterTest, RejectsSymbolLessInjectedOrderWithoutFallbackSymbol) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnInjectedOrder(MakeInjectedOrder("", true, 100.0, 1000));

    EXPECT_EQ(adapter.SymbolCount(), 0U);
    EXPECT_EQ(adapter.Metrics().submitted, 0U);
    EXPECT_EQ(adapter.Metrics().unsupported, 1U);
}

TEST(MultiSymbolReplayAdapterTest, CreatesNewBookForInjectedOrderSymbol) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnInjectedOrder(MakeInjectedOrder("SOL", true, 150.0, 1000));

    ASSERT_EQ(adapter.SymbolCount(), 1U);
    EXPECT_NE(adapter.FindEngine("SOL"), nullptr);

    const MatchingEngine *sol_engine = adapter.FindEngine("SOL");
    ASSERT_NE(sol_engine, nullptr);

    const auto best_bid = sol_engine->Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 150.0);

    EXPECT_EQ(adapter.Metrics().submitted, 1U);
    EXPECT_EQ(adapter.Metrics().unsupported, 0U);
}

TEST(MultiSymbolReplayAdapterTest, ReturnsSymbolsInSortedOrder) {
    MultiSymbolReplayAdapter adapter;

    adapter.OnEvent(MakeEvent("SOL", EventType::New, false, 100.0, 0.01));
    adapter.OnEvent(MakeEvent("BTC", EventType::New, false, 101.0, 0.01));
    adapter.OnEvent(MakeEvent("ETH", EventType::New, false, 102.0, 0.01));

    const std::vector<std::string> expected{
        "BTC",
        "ETH",
        "SOL",
    };

    EXPECT_EQ(adapter.Symbols(), expected);
}

} // namespace
} // namespace bookforge
