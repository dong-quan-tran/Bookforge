#include "replay/StrategyExperimentRunner.hpp"

#include <gtest/gtest.h>

namespace bookforge {
namespace {

ExternalOrderEvent MakeNewEvent(bool is_ask, double price, double size) {
    ExternalOrderEvent event{};
    event.eventType = EventType::New;
    event.isAsk = is_ask;
    event.price = price;
    event.size = size;
    return event;
}

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

TEST(StrategyExperimentRunnerTest, UnfilledInjectedOrderLeavesExecutionMetricsEmpty) {
    ReplayConfig replay_config;
    StrategyExperimentRunner runner(replay_config);

    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 0;
    config.timing = InjectedOrderTiming::AfterEvent;
    config.is_buy = true;
    config.limit_price = 101.0;
    config.quantity = 3;

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(true, 102.0, 0.00003),
    };

    const auto result = runner.RunOnce(config, events, "experiment-order", "experiment");

    EXPECT_EQ(result.requested_qty, 3U);
    EXPECT_EQ(result.filled_qty, 0U);
    EXPECT_EQ(result.remaining_qty, 3U);
    EXPECT_DOUBLE_EQ(result.fill_rate, 0.0);
    EXPECT_DOUBLE_EQ(result.avg_execution_price, 0.0);
}

TEST(StrategyExperimentRunnerTest, InjectedOrderFillsUpdateExperimentResult) {
    ReplayConfig replay_config;
    StrategyExperimentRunner runner(replay_config);

    StrategyExperimentConfig config;
    config.mode = StrategyMode::Aggressive;
    config.entry_offset = 0;
    config.timing = InjectedOrderTiming::AfterEvent;
    config.is_buy = true;
    config.limit_price = 101.0;
    config.quantity = 3;

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(true, 100.0, 0.00002),
    };

    const auto result = runner.RunOnce(config, events, "experiment-order", "experiment");

    EXPECT_EQ(result.requested_qty, 3U);
    EXPECT_EQ(result.filled_qty, 2U);
    EXPECT_EQ(result.remaining_qty, 1U);
    EXPECT_DOUBLE_EQ(result.fill_rate, 2.0 / 3.0);
    EXPECT_DOUBLE_EQ(result.avg_execution_price, 100.0);
}

TEST(StrategyExperimentRunnerTest, AfterEventCapturesBookAfterConfiguredEntryEvent) {
    ReplayConfig replay_config;
    StrategyExperimentRunner runner(replay_config);

    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 1;
    config.timing = InjectedOrderTiming::AfterEvent;
    config.is_buy = true;
    config.limit_price = 100.0;
    config.quantity = 1;

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(false, 99.0, 0.00005),
        MakeNewEvent(true, 101.0, 0.00004),
    };

    const auto result = runner.RunOnce(config, events, "experiment-order", "experiment");

    ASSERT_TRUE(result.decision_best_bid.has_value());
    ASSERT_TRUE(result.decision_best_ask.has_value());
    EXPECT_DOUBLE_EQ(*result.decision_best_bid, 99.0);
    EXPECT_DOUBLE_EQ(*result.decision_best_ask, 101.0);
    EXPECT_DOUBLE_EQ(result.decision_mid_price, 100.0);
    EXPECT_DOUBLE_EQ(result.decision_spread, 2.0);
    EXPECT_TRUE(result.has_decision_metrics);
}

TEST(StrategyExperimentRunnerTest, BeforeEventCapturesBookBeforeConfiguredEntryEvent) {
    ReplayConfig replay_config;
    StrategyExperimentRunner runner(replay_config);

    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 1;
    config.timing = InjectedOrderTiming::BeforeEvent;
    config.is_buy = true;
    config.limit_price = 100.0;
    config.quantity = 1;

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(false, 99.0, 0.00005),
        MakeNewEvent(true, 101.0, 0.00004),
    };

    const auto result = runner.RunOnce(config, events, "experiment-order", "experiment");

    ASSERT_TRUE(result.decision_best_bid.has_value());
    EXPECT_DOUBLE_EQ(*result.decision_best_bid, 99.0);
    EXPECT_FALSE(result.decision_best_ask.has_value());
    EXPECT_DOUBLE_EQ(result.decision_mid_price, 0.0);
    EXPECT_DOUBLE_EQ(result.decision_spread, 0.0);
    EXPECT_FALSE(result.has_decision_metrics);
}

TEST(StrategyExperimentRunnerTest, OneSidedBookLeavesMidAndSpreadUnavailable) {
    ReplayConfig replay_config;
    StrategyExperimentRunner runner(replay_config);

    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 1;
    config.timing = InjectedOrderTiming::AfterEvent;
    config.is_buy = true;
    config.limit_price = 98.0;
    config.quantity = 1;

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(false, 99.0, 0.00005),
        MakeNewEvent(false, 98.0, 0.00004),
    };

    const auto result = runner.RunOnce(config, events, "experiment-order", "experiment");

    ASSERT_TRUE(result.decision_best_bid.has_value());
    EXPECT_DOUBLE_EQ(*result.decision_best_bid, 99.0);
    EXPECT_FALSE(result.decision_best_ask.has_value());
    EXPECT_DOUBLE_EQ(result.decision_mid_price, 0.0);
    EXPECT_DOUBLE_EQ(result.decision_spread, 0.0);
    EXPECT_FALSE(result.has_decision_metrics);
}

} // namespace
} // namespace bookforge