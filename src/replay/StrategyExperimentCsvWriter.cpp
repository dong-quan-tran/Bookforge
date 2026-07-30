#include "replay/StrategyExperimentCsvWriter.hpp"

#include <fstream>
#include <string>

namespace bookforge {
namespace {

std::string StrategyModeToString(StrategyMode mode) {
  switch (mode) {
    case StrategyMode::Passive:
      return "passive";
    case StrategyMode::Aggressive:
      return "aggressive";
  }
  return "unknown";
}

std::string EscapeCsv(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size() + 2);

  for (const char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped += ch;
    }
  }

  return "\"" + escaped + "\"";
}

}  // namespace

bool StrategyExperimentCsvWriter::Write(
    const std::string& path,
    const std::vector<StrategyExperimentResult>& results) {
  std::ofstream out(path);
  if (!out.is_open()) {
    return false;
  }

  out << "strategy,entry_offset,is_buy,limit_price,requested_qty,filled_qty,"
         "remaining_qty,fill_rate,avg_execution_price,decision_mid_price,"
         "decision_spread,implementation_shortfall_bps,"
         "time_to_first_fill_us,time_to_full_fill_us\n";

  for (const auto& result : results) {
    out << EscapeCsv(StrategyModeToString(result.mode)) << ","
        << result.entry_offset << "," << (result.is_buy ? "true" : "false")
        << "," << result.limit_price << "," << result.requested_qty << ","
        << result.filled_qty << "," << result.remaining_qty << ","
        << result.fill_rate << "," << result.avg_execution_price << ","
        << result.decision_mid_price << "," << result.decision_spread << ","
        << result.implementation_shortfall_bps << ","
        << result.time_to_first_fill_us << ","
        << result.time_to_full_fill_us << "\n";
  }

  return true;
}

}  // namespace bookforge