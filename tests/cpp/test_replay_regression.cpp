#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "HyperliquidCsvReader.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

namespace bookforge {
namespace {

#ifndef BOOKFORGE_TEST_FIXTURE_DIR
#error "BOOKFORGE_TEST_FIXTURE_DIR is not defined"
#endif

std::string FixturePath(const std::string &filename) {
    return (std::filesystem::path(BOOKFORGE_TEST_FIXTURE_DIR) / filename).string();
}

TEST(ReplayRegressionTest, BasicFixtureProducesStableReplayOutcome) {
    const std::string path = FixturePath("hyperliquid_replay_fixture_basic.csv");

    HyperliquidCsvReader reader(path);
    std::vector<ExternalOrderEvent> events = reader.read_all(false, true);

    ReplayConfig config;
    config.path = path;
    config.symbol = "TEST";
    config.source = ReplaySource::Hyperliquid;
    config.log_summary = false;
    config.log_every_n = 0;
    config.strict_mode = false;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    const bool ok = runner.Run(adapter, events);

    ASSERT_TRUE(ok);
    EXPECT_EQ(adapter.Stats().totalEvents, events.size());
    EXPECT_GE(adapter.Stats().submittedOrders, static_cast<std::size_t>(1));

    const auto best_bid = engine.Book().GetBestBid();
    const auto best_ask = engine.Book().GetBestAsk();
    EXPECT_TRUE(best_bid.has_value() || best_ask.has_value());
}

TEST(ReplayRegressionTest, BoundedFixtureReplayProducesStableSubrangeOutcome) {
    const std::string path = FixturePath("hyperliquid_replay_fixture_bounded.csv");

    HyperliquidCsvReader reader(path);
    std::vector<ExternalOrderEvent> events = reader.read_all(false, true);

    ReplayConfig config;
    config.path = path;
    config.symbol = "TEST";
    config.source = ReplaySource::Hyperliquid;
    config.start_offset = 1;
    config.max_events = 2;
    config.log_summary = false;
    config.log_every_n = 0;
    config.strict_mode = false;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    const bool ok = runner.Run(adapter, events);

    ASSERT_TRUE(ok);
    EXPECT_LE(adapter.Stats().totalEvents, static_cast<std::size_t>(2));
}

} // namespace
} // namespace bookforge