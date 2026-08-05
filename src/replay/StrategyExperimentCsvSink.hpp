#pragma once

#include <string>
#include <vector>

#include "replay/StrategyExperimentCsvWriter.hpp"
#include "replay/StrategyExperimentSink.hpp"

namespace bookforge {

class StrategyExperimentCsvSink final : public StrategyExperimentSink {
  public:
    explicit StrategyExperimentCsvSink(std::string output_path);

    void OnResult(const StrategyExperimentResult &result) override;
    bool Flush();

  private:
    std::string output_path_;
    std::vector<StrategyExperimentResult> results_;
};

} // namespace bookforge