#include "replay/StrategyExperimentCsvSink.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(StrategyExperimentCsvSinkTest, FlushWritesSingleResult) {
  const auto output_path =
      std::filesystem::temp_directory_path() / "strategy_sink_test.csv";

  StrategyExperimentCsvSink sink(output_path.string());

  StrategyExperimentResult result;
  result.mode = StrategyMode::Passive;
  result.entry_offset = 7;
  result.is_buy = true;
  result.limit_price = 100.5;
  result.requested_qty = 2;
  result.filled_qty = 1;

  sink.OnResult(result);
  ASSERT_TRUE(sink.Flush());

  std::ifstream in(output_path);
  ASSERT_TRUE(in.is_open());

  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string content = buffer.str();

  EXPECT_NE(content.find("\"passive\",7,true,100.5,2,1"),
            std::string::npos);

  in.close();
  std::error_code ec;
  std::filesystem::remove(output_path, ec);
}

}  // namespace
}  // namespace bookforge