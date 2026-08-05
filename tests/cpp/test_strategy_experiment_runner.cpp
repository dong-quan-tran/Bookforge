#include "replay/StrategyExperimentRunner.hpp"

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(StrategyExperimentRunnerTest, PlaceholderConstructsAndRuns) {
    ReplayConfig replay_config;
    StrategyExperimentRunner runner(replay_config);

    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 0;
    config.is_buy = true;
    config.limit_price = 100.0;
    config.quantity = 1;

    const std::vector<ExternalOrderEvent> events;
    const auto result = runner.RunOnce(config, events, "order-1", "p1");

    EXPECT_EQ(result.mode, StrategyMode::Passive);
    EXPECT_EQ(result.entry_offset, 0U);
    EXPECT_TRUE(result.is_buy);
    EXPECT_DOUBLE_EQ(result.limit_price, 100.0);
    EXPECT_EQ(result.requested_qty, 1U);
    EXPECT_EQ(result.remaining_qty, 1U);
}

} // namespace
} // namespace bookforge