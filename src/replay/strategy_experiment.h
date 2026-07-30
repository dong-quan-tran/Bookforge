#pragma once

#include <cstdint>
#include <string>

namespace bookforge::replay {

enum class StrategyMode {
  Passive,
  Aggressive,
};

struct StrategyExperimentConfig {
  StrategyMode mode{StrategyMode::Passive};
  std::string csv_path{};
  std::uint64_t entry_offset{0};
  std::uint64_t parent_qty{0};
  bool is_buy{true};
};

struct StrategyExperimentResult {
  StrategyMode mode{StrategyMode::Passive};
  std::uint64_t parent_qty{0};
  std::uint64_t filled_qty{0};
  std::uint64_t remaining_qty{0};

  double fill_rate{0.0};
  double avg_execution_price{0.0};
  double decision_mid_price{0.0};
  double decision_spread{0.0};
  double implementation_shortfall_bps{0.0};

  std::uint64_t time_to_first_fill_us{0};
  std::uint64_t time_to_full_fill_us{0};
};

}  // namespace bookforge::replay