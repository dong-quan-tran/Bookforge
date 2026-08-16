#include "replay/StrategyExperimentComparison.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "replay/StrategyExperimentCsvWriter.hpp"

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

StrategyExperimentConfig MakePassiveConfig() {
    StrategyExperimentConfig config;
    config.mode = StrategyMode::Passive;
    config.entry_offset = 1;
    config.timing = InjectedOrderTiming::AfterEvent;
    config.is_buy = true;
    config.limit_price = 99.0;
    config.quantity = 2;
    return config;
}

StrategyExperimentConfig MakeAggressiveConfig() {
    StrategyExperimentConfig config;
    config.mode = StrategyMode::Aggressive;
    config.entry_offset = 1;
    config.timing = InjectedOrderTiming::AfterEvent;
    config.is_buy = true;
    config.limit_price = 101.0;
    config.quantity = 2;
    return config;
}

TEST(StrategyExperimentComparisonRunnerTest,
     RunsPassiveAndAggressiveConfigurationsAgainstSameReplayEvents) {
    ReplayConfig replay_config;
    StrategyExperimentComparisonRunner runner(replay_config);

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(false, 99.0, 0.00005),
        MakeNewEvent(true, 100.0, 0.00002),
    };

    StrategyExperimentComparisonResult result;

    ASSERT_TRUE(runner.Run(MakePassiveConfig(), MakeAggressiveConfig(), events, "passive-order",
                           "aggressive-order", "comparison", result));

    EXPECT_EQ(result.passive.mode, StrategyMode::Passive);
    EXPECT_EQ(result.aggressive.mode, StrategyMode::Aggressive);

    EXPECT_EQ(result.passive.entry_offset, 1U);
    EXPECT_EQ(result.aggressive.entry_offset, 1U);

    EXPECT_EQ(result.passive.requested_qty, 2U);
    EXPECT_EQ(result.aggressive.requested_qty, 2U);

    EXPECT_EQ(result.passive.filled_qty, 0U);
    EXPECT_EQ(result.passive.remaining_qty, 2U);
    EXPECT_DOUBLE_EQ(result.passive.fill_rate, 0.0);

    EXPECT_EQ(result.aggressive.filled_qty, 2U);
    EXPECT_EQ(result.aggressive.remaining_qty, 0U);
    EXPECT_DOUBLE_EQ(result.aggressive.fill_rate, 1.0);
    EXPECT_DOUBLE_EQ(result.aggressive.avg_execution_price, 100.0);
}

TEST(StrategyExperimentComparisonRunnerTest, RejectsMismatchedEntryOffsets) {
    ReplayConfig replay_config;
    StrategyExperimentComparisonRunner runner(replay_config);

    StrategyExperimentConfig passive = MakePassiveConfig();
    StrategyExperimentConfig aggressive = MakeAggressiveConfig();
    aggressive.entry_offset = passive.entry_offset + 1;

    StrategyExperimentComparisonResult result;
    const std::vector<ExternalOrderEvent> events;

    EXPECT_FALSE(runner.Run(passive, aggressive, events, "passive-order", "aggressive-order",
                            "comparison", result));
}

TEST(StrategyExperimentComparisonRunnerTest, WritesTwoReproducibleCsvRows) {
    ReplayConfig replay_config;
    StrategyExperimentComparisonRunner runner(replay_config);

    const std::vector<ExternalOrderEvent> events{
        MakeNewEvent(false, 99.0, 0.00005),
        MakeNewEvent(true, 100.0, 0.00002),
    };

    StrategyExperimentComparisonResult result;

    ASSERT_TRUE(runner.Run(MakePassiveConfig(), MakeAggressiveConfig(), events, "passive-order",
                           "aggressive-order", "comparison", result));

    const auto output_path =
        std::filesystem::temp_directory_path() / "strategy_experiment_comparison_test.csv";

    const std::vector<StrategyExperimentResult> results{
        result.passive,
        result.aggressive,
    };

    ASSERT_TRUE(StrategyExperimentCsvWriter::Write(output_path.string(), results));

    std::ifstream input(output_path);
    ASSERT_TRUE(input.is_open());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    input.close();

    const std::string content = buffer.str();

    EXPECT_NE(content.find("strategy,entry_offset,is_buy,limit_price"), std::string::npos);
    EXPECT_NE(content.find("\"passive\",1,true,99,2,0,2"), std::string::npos);
    EXPECT_NE(content.find("\"aggressive\",1,true,101,2,2,0"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(output_path, ec);
}

} // namespace
} // namespace bookforge