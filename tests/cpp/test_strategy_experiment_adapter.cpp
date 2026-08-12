#include "replay/StrategyExperimentAdapter.hpp"

#include <gtest/gtest.h>

namespace bookforge {
namespace {

StrategyExperimentConfig MakeConfig(std::uint32_t quantity) {
    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 5;
    config.is_buy = true;
    config.limit_price = 100.0;
    config.quantity = quantity;
    return config;
}

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
    EXPECT_FALSE(result.decision_best_bid.has_value());
    EXPECT_FALSE(result.decision_best_ask.has_value());
    EXPECT_FALSE(result.has_decision_metrics);
}

TEST(StrategyExperimentAdapterTest, NoFillLeavesExecutionMetricsAtDefaults) {
    StrategyExperimentAdapter adapter(MakeConfig(5));

    const auto result = adapter.Result();

    EXPECT_EQ(result.filled_qty, 0U);
    EXPECT_EQ(result.remaining_qty, 5U);
    EXPECT_DOUBLE_EQ(result.fill_rate, 0.0);
    EXPECT_DOUBLE_EQ(result.avg_execution_price, 0.0);
}

TEST(StrategyExperimentAdapterTest, PartialFillUpdatesResult) {
    StrategyExperimentAdapter adapter(MakeConfig(5));

    adapter.OnFill(3, 100.25);

    const auto result = adapter.Result();

    EXPECT_EQ(result.filled_qty, 3U);
    EXPECT_EQ(result.remaining_qty, 2U);
    EXPECT_DOUBLE_EQ(result.fill_rate, 0.6);
    EXPECT_DOUBLE_EQ(result.avg_execution_price, 100.25);
}

TEST(StrategyExperimentAdapterTest, MultipleFillsUseWeightedAverageExecutionPrice) {
    StrategyExperimentAdapter adapter(MakeConfig(10));

    adapter.OnFill(3, 100.0);
    adapter.OnFill(2, 103.0);

    const auto result = adapter.Result();

    EXPECT_EQ(result.filled_qty, 5U);
    EXPECT_EQ(result.remaining_qty, 5U);
    EXPECT_DOUBLE_EQ(result.fill_rate, 0.5);
    EXPECT_NEAR(result.avg_execution_price, 101.2, 1e-12);
}

TEST(StrategyExperimentAdapterTest, FillQuantityIsClampedToRequestedQuantity) {
    StrategyExperimentAdapter adapter(MakeConfig(5));

    adapter.OnFill(3, 100.0);
    adapter.OnFill(10, 110.0);

    const auto result = adapter.Result();

    EXPECT_EQ(result.filled_qty, 5U);
    EXPECT_EQ(result.remaining_qty, 0U);
    EXPECT_DOUBLE_EQ(result.fill_rate, 1.0);
    EXPECT_DOUBLE_EQ(result.avg_execution_price, 104.0);
}

TEST(StrategyExperimentAdapterTest, CapturesTwoSidedDecisionBookState) {
    StrategyExperimentAdapter adapter(MakeConfig(3));

    TopOfBookSnapshot snapshot{};
    snapshot.best_bid = 99.0;
    snapshot.best_ask = 101.0;
    snapshot.mid_price = 100.0;
    snapshot.spread = 2.0;

    adapter.CaptureDecisionBookState(snapshot);

    const auto result = adapter.Result();

    ASSERT_TRUE(result.decision_best_bid.has_value());
    ASSERT_TRUE(result.decision_best_ask.has_value());
    EXPECT_DOUBLE_EQ(*result.decision_best_bid, 99.0);
    EXPECT_DOUBLE_EQ(*result.decision_best_ask, 101.0);
    EXPECT_DOUBLE_EQ(result.decision_mid_price, 100.0);
    EXPECT_DOUBLE_EQ(result.decision_spread, 2.0);
    EXPECT_TRUE(result.has_decision_metrics);
}

TEST(StrategyExperimentAdapterTest, EmptyBookSideLeavesMidAndSpreadUnavailable) {
    StrategyExperimentAdapter adapter(MakeConfig(3));

    TopOfBookSnapshot snapshot{};
    snapshot.best_bid = 99.0;

    adapter.CaptureDecisionBookState(snapshot);

    const auto result = adapter.Result();

    ASSERT_TRUE(result.decision_best_bid.has_value());
    EXPECT_DOUBLE_EQ(*result.decision_best_bid, 99.0);
    EXPECT_FALSE(result.decision_best_ask.has_value());
    EXPECT_DOUBLE_EQ(result.decision_mid_price, 0.0);
    EXPECT_DOUBLE_EQ(result.decision_spread, 0.0);
    EXPECT_FALSE(result.has_decision_metrics);
}

TEST(StrategyExperimentAdapterTest, CapturesDecisionBookStateOnlyOnce) {
    StrategyExperimentAdapter adapter(MakeConfig(3));

    TopOfBookSnapshot first{};
    first.best_bid = 99.0;
    first.best_ask = 101.0;
    first.mid_price = 100.0;
    first.spread = 2.0;

    TopOfBookSnapshot second{};
    second.best_bid = 98.0;
    second.best_ask = 102.0;
    second.mid_price = 100.0;
    second.spread = 4.0;

    adapter.CaptureDecisionBookState(first);
    adapter.CaptureDecisionBookState(second);

    const auto result = adapter.Result();

    ASSERT_TRUE(result.decision_best_bid.has_value());
    ASSERT_TRUE(result.decision_best_ask.has_value());
    EXPECT_DOUBLE_EQ(*result.decision_best_bid, 99.0);
    EXPECT_DOUBLE_EQ(*result.decision_best_ask, 101.0);
    EXPECT_DOUBLE_EQ(result.decision_mid_price, 100.0);
    EXPECT_DOUBLE_EQ(result.decision_spread, 2.0);
}

} // namespace
} // namespace bookforge