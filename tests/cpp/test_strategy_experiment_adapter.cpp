#include "replay/StrategyExperimentAdapter.hpp"

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(StrategyExperimentAdapterTest, InitializesResultFromConfig) {
    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 10;
    config.is_buy = true;
    config.limit_price = 101.5;
    config.quantity = 7;

    StrategyExperimentAdapter adapter(config);
    const auto result = adapter.Result();

    EXPECT_EQ(result.mode, StrategyMode::Passive);
    EXPECT_EQ(result.entry_offset, 10U);
    EXPECT_TRUE(result.is_buy);
    EXPECT_DOUBLE_EQ(result.limit_price, 101.5);
    EXPECT_EQ(result.requested_qty, 7U);
    EXPECT_EQ(result.remaining_qty, 7U);
    EXPECT_FALSE(result.has_decision_metrics);
}

TEST(StrategyExperimentAdapterTest, MarksDecisionMetricsCapturedOnFirstEvent) {
    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 5;
    config.is_buy = true;
    config.limit_price = 100.0;
    config.quantity = 3;

    StrategyExperimentAdapter adapter(config);

    ExternalOrderEvent event{};
    // No need to set any fields; adapter currently captures on first event.

    auto before = adapter.Result();
    EXPECT_FALSE(before.has_decision_metrics);

    adapter.OnEvent(event);

    auto after = adapter.Result();
    EXPECT_TRUE(after.has_decision_metrics);
}

} // namespace
} // namespace bookforge