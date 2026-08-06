#include <iostream>
#include <string>
#include <vector>

#include "HyperliquidCsvReader.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/StrategyExperiment.hpp"
#include "replay/StrategyExperimentCsvWriter.hpp"
#include "replay/StrategyExperimentRunner.hpp"

namespace bookforge {
namespace {

struct CliOptions {
    std::string input_csv;
    std::string output_csv{"output/strategy_experiment_results.csv"};
    std::string mode{"passive"};
    std::string side{"buy"};
    std::uint32_t quantity{1};
    std::uint64_t entry_offset{0};
};

bool ParseArgs(int argc, char **argv, CliOptions &opts) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);

        if (arg == "--input" && i + 1 < argc) {
            opts.input_csv = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            opts.output_csv = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            opts.mode = argv[++i];
        } else if (arg == "--side" && i + 1 < argc) {
            opts.side = argv[++i];
        } else if (arg == "--quantity" && i + 1 < argc) {
            opts.quantity = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--entry-offset" && i + 1 < argc) {
            opts.entry_offset = static_cast<std::uint64_t>(std::stoull(argv[++i]));
        } else if (arg == "--help") {
            return false;
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            return false;
        }
    }

    if (opts.input_csv.empty()) {
        std::cerr << "Missing required --input argument.\n";
        return false;
    }

    return true;
}

void PrintUsage() {
    std::cout << "Usage:\n"
              << "  strategy_experiment_main --input <csv> [--output <csv>]\n"
              << "                          [--mode passive|aggressive]\n"
              << "                          [--side buy|sell]\n"
              << "                          [--quantity N]\n"
              << "                          [--entry-offset N]\n";
}

StrategyMode ParseMode(const std::string &mode) {
    if (mode == "aggressive") {
        return StrategyMode::Aggressive;
    }
    return StrategyMode::Passive;
}

bool ParseSide(const std::string &side) {
    return side != "sell";
}

StrategyExperimentConfig BuildExperimentConfig(const CliOptions &opts) {
    StrategyExperimentConfig config;
    config.mode = ParseMode(opts.mode);
    config.csv_path = opts.input_csv;
    config.entry_offset = opts.entry_offset;
    config.is_buy = ParseSide(opts.side);
    config.limit_price = 0.0; // Filled by adapter later if needed.
    config.quantity = opts.quantity;
    config.timing = InjectedOrderTiming::BeforeEvent;
    return config;
}

ReplayConfig BuildReplayConfig(const CliOptions &opts) {
    ReplayConfig config;
    config.start_offset = static_cast<std::uint64_t>(opts.entry_offset);
    config.max_events = 0;      // Process to end.
    config.log_every_n = 0;     // No periodic logging.
    config.log_summary = false; // Keep CLI output simple for now.
    return config;
}

} // namespace
} // namespace bookforge

int main(int argc, char **argv) {
    using namespace bookforge;

    CliOptions opts;
    if (!ParseArgs(argc, argv, opts)) {
        PrintUsage();
        return 1;
    }

    std::cout << "strategy_experiment_main\n"
              << "  input: " << opts.input_csv << '\n'
              << "  output: " << opts.output_csv << '\n'
              << "  mode: " << opts.mode << '\n'
              << "  side: " << opts.side << '\n'
              << "  quantity: " << opts.quantity << '\n'
              << "  entry_offset: " << opts.entry_offset << '\n';

    HyperliquidCsvReader reader;
    const auto events = reader.Read(opts.input_csv);

    const auto experiment_config = BuildExperimentConfig(opts);
    const auto replay_config = BuildReplayConfig(opts);

    StrategyExperimentRunner runner(replay_config);
    const auto result = runner.RunOnce(experiment_config, events, "experiment-order-1", "cli");

    std::vector<StrategyExperimentResult> results;
    results.push_back(result);

    if (!StrategyExperimentCsvWriter::Write(opts.output_csv, results)) {
        std::cerr << "Failed to write experiment results CSV: " << opts.output_csv << '\n';
        return 1;
    }

    std::cout << "Wrote experiment result to: " << opts.output_csv << '\n';
    return 0;
}