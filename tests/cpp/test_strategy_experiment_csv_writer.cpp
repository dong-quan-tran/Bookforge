#include "replay/StrategyExperimentCsvWriter.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(StrategyExperimentCsvWriterTest, WritesHeaderAndRows) {
    const auto output_path =
        std::filesystem::temp_directory_path() / "strategy_experiment_results_test.csv";

    StrategyExperimentResult result;
    result.mode = StrategyMode::Passive;
    result.entry_offset = 123;
    result.is_buy = true;
    result.limit_price = 101.5;
    result.requested_qty = 10;
    result.filled_qty = 6;
    result.remaining_qty = 4;
    result.fill_rate = 0.6;
    result.avg_execution_price = 101.25;
    result.decision_mid_price = 101.0;
    result.decision_spread = 0.5;
    result.implementation_shortfall_bps = 12.5;
    result.time_to_first_fill_us = 50;
    result.time_to_full_fill_us = 0;

    const std::vector<StrategyExperimentResult> results{result};

    ASSERT_TRUE(StrategyExperimentCsvWriter::Write(output_path.string(), results));

    std::ifstream in(output_path);
    ASSERT_TRUE(in.is_open());

    std::ostringstream buffer;
    buffer << in.rdbuf();
    in.close(); // Ensure file is closed before removal on Windows.

    const std::string content = buffer.str();

    EXPECT_NE(content.find("strategy,entry_offset,is_buy,limit_price"), std::string::npos);
    EXPECT_NE(content.find("\"passive\",123,true,101.5,10,6,4,0.6,101.25,101"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(output_path, ec);
}

TEST(StrategyExperimentCsvWriterTest, WritesAggressiveModeString) {
    const auto output_path =
        std::filesystem::temp_directory_path() / "strategy_experiment_results_mode_test.csv";

    StrategyExperimentResult result;
    result.mode = StrategyMode::Aggressive;
    result.entry_offset = 5;
    result.is_buy = false;
    result.limit_price = 99.0;
    result.requested_qty = 3;

    const std::vector<StrategyExperimentResult> results{result};

    ASSERT_TRUE(StrategyExperimentCsvWriter::Write(output_path.string(), results));

    std::ifstream in(output_path);
    ASSERT_TRUE(in.is_open());

    std::ostringstream buffer;
    buffer << in.rdbuf();
    in.close(); // Ensure file is closed before removal on Windows.

    const std::string content = buffer.str();

    EXPECT_NE(content.find("\"aggressive\",5,false,99,3"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(output_path, ec);
}

} // namespace
} // namespace bookforge