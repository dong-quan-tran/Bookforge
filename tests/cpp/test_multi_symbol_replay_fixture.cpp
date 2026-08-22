#include "HyperliquidCsvReader.hpp"
#include "replay/MultiSymbolReplayAdapter.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"
#include "replay/SymbolReplayFilter.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

const std::string kFixturePath =
    std::string(BOOKFORGE_TEST_FIXTURE_DIR) + "/hyperliquid_multi_symbol_fixture.csv";

ReplayConfig MakeReplayConfig() {
    ReplayConfig config;
    config.path = kFixturePath;
    config.symbol = "BTCUSDT.P";
    config.source = ReplaySource::Hyperliquid;
    config.max_events = 0;
    config.start_offset = 0;
    config.pacing_mode = ReplayPacingMode::Unpaced;
    config.replay_speed = 1.0;
    config.log_every_n = 0;
    config.log_summary = false;
    config.log_errors = true;
    config.strict_mode = true;
    return config;
}

TEST(MultiSymbolReplayFixtureTest, BuildsIndependentFinalBooksForEachSymbol) {
    const ReplayConfig config = MakeReplayConfig();

    HyperliquidCsvReader reader(config.path);
    const std::vector<ExternalOrderEvent> events = reader.read_all(true, true);

    ASSERT_EQ(events.size(), 6U);

    MultiSymbolReplayAdapter adapter(config.symbol);
    ReplayRunner runner(config);

    ASSERT_TRUE(runner.Run(adapter, events));

    EXPECT_EQ(adapter.SymbolCount(), 2U);

    const MatchingEngine *btc_engine = adapter.FindEngine("BTCUSDT.P");
    const MatchingEngine *eth_engine = adapter.FindEngine("ETHUSDT.P");

    ASSERT_NE(btc_engine, nullptr);
    ASSERT_NE(eth_engine, nullptr);

    const auto btc_best_bid = btc_engine->Book().GetBestBid();
    const auto btc_best_ask = btc_engine->Book().GetBestAsk();
    const auto eth_best_bid = eth_engine->Book().GetBestBid();
    const auto eth_best_ask = eth_engine->Book().GetBestAsk();

    ASSERT_TRUE(btc_best_bid.has_value());
    ASSERT_TRUE(btc_best_ask.has_value());
    ASSERT_TRUE(eth_best_bid.has_value());
    ASSERT_TRUE(eth_best_ask.has_value());

    EXPECT_DOUBLE_EQ(*btc_best_bid, 99.0);
    EXPECT_DOUBLE_EQ(*btc_best_ask, 100.0);
    EXPECT_DOUBLE_EQ(*eth_best_bid, 89.0);
    EXPECT_DOUBLE_EQ(*eth_best_ask, 90.0);

    EXPECT_EQ(adapter.Symbols(), (std::vector<std::string>{"BTCUSDT.P", "ETHUSDT.P"}));
}

TEST(MultiSymbolReplayFixtureTest, SymbolFilterRetainsOnlySelectedBookEvents) {
    const ReplayConfig config = MakeReplayConfig();

    HyperliquidCsvReader reader(config.path);
    const std::vector<ExternalOrderEvent> events = reader.read_all(true, true);
    const std::vector<ExternalOrderEvent> btc_events =
        FilterReplayEventsBySymbol(events, "BTCUSDT.P", config.symbol);

    ASSERT_EQ(btc_events.size(), 3U);

    for (const ExternalOrderEvent &event : btc_events) {
        EXPECT_EQ(event.symbol, "BTCUSDT.P");
    }

    MultiSymbolReplayAdapter adapter(config.symbol);
    ReplayRunner runner(config);

    ASSERT_TRUE(runner.Run(adapter, btc_events));

    EXPECT_EQ(adapter.SymbolCount(), 1U);
    EXPECT_NE(adapter.FindEngine("BTCUSDT.P"), nullptr);
    EXPECT_EQ(adapter.FindEngine("ETHUSDT.P"), nullptr);

    const MatchingEngine *btc_engine = adapter.FindEngine("BTCUSDT.P");
    ASSERT_NE(btc_engine, nullptr);

    const auto best_bid = btc_engine->Book().GetBestBid();
    const auto best_ask = btc_engine->Book().GetBestAsk();

    ASSERT_TRUE(best_bid.has_value());
    ASSERT_TRUE(best_ask.has_value());

    EXPECT_DOUBLE_EQ(*best_bid, 99.0);
    EXPECT_DOUBLE_EQ(*best_ask, 100.0);
}

} // namespace
} // namespace bookforge
