#include "replay/StrategyExperimentCsvSink.hpp"

#include <utility>

namespace bookforge {

StrategyExperimentCsvSink::StrategyExperimentCsvSink(std::string output_path)
    : output_path_(std::move(output_path)) {}

void StrategyExperimentCsvSink::OnResult(const StrategyExperimentResult &result) {
    results_.push_back(result);
}

bool StrategyExperimentCsvSink::Flush() {
    return StrategyExperimentCsvWriter::Write(output_path_, results_);
}

} // namespace bookforge