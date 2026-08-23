#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "HyperliquidCsvReader.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/StrategyExperiment.hpp"
#include "replay/StrategyExperimentCsvWriter.hpp"
#include "replay/StrategyExperimentRunner.hpp"
#include "replay/SymbolReplayFilter.hpp"

namespace bookforge {
namespace {

constexpr const char *kFallbackSymbol = "BTCUSDT.P";

struct CliOptions {
    std::string input_csv;
    std::string output_csv{"output/strategy_experiment_results.csv"};
    std::string symbol;
    std::string mode{"passive"};
    std::string side{"buy"};
    double limit_price{0.0};
    std::uint32_t quantity{1};
    std::uint64_t entry_offset{0};
};

bool ParseMode(const std::string &value, StrategyMode &mode) {
    if (value == "passive") {
        mode = StrategyMode::Passive;
        return true;
    }

    if (value == "aggressive") {
        mode = StrategyMode::Aggressive;
        return true;
    }

    return false;
}

bool ParseSide(const std::string &value, bool &is_buy) {
    if (value == "buy") {
        is_buy = true;
        return true;
    }

    if (value == "sell") {
        is_buy = false;
        return true;
    }

    return false;
}

bool ParseArgs(int argc, char **argv, CliOptions &opts) {
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);

            if (arg == "--input") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --input.\n";
                    return false;
                }

                opts.input_csv = argv[++i];
            } else if (arg == "--output") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --output.\n";
                    return false;
                }

                opts.output_csv = argv[++i];
            } else if (arg == "--symbol") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --symbol.\n";
                    return false;
                }

                opts.symbol = argv[++i];
                if (opts.symbol.empty()) {
                    std::cerr << "Invalid --symbol value. Expected a non-empty symbol.\n";
                    return false;
                }
            } else if (arg == "--mode") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --mode.\n";
                    return false;
                }

                opts.mode = argv[++i];
            } else if (arg == "--side") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --side.\n";
                    return false;
                }

                opts.side = argv[++i];
            } else if (arg == "--limit-price") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --limit-price.\n";
                    return false;
                }

                opts.limit_price = std::stod(argv[++i]);
            } else if (arg == "--quantity") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --quantity.\n";
                    return false;
                }

                opts.quantity = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            } else if (arg == "--entry-offset") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --entry-offset.\n";
                    return false;
                }

                opts.entry_offset = static_cast<std::uint64_t>(std::stoull(argv[++i]));
            } else if (arg == "--help" || arg == "-h") {
                return false;
            } else {
                std::cerr << "Unknown argument: " << arg << '\n';
                return false;
            }
        }
    } catch (const std::exception &) {
        std::cerr << "Invalid numeric argument.\n";
        return false;
    }

    if (opts.input_csv.empty()) {
        std::cerr << "Missing required --input argument.\n";
        return false;
    }

    if (opts.limit_price <= 0.0) {
        std::cerr << "Invalid --limit-price value. Expected a positive number.\n";
        return false;
    }

    if (opts.quantity == 0) {
        std::cerr << "Invalid --quantity value. Expected a positive integer.\n";
        return false;
    }

    return true;
}

void PrintUsage() {
    std::cout << "Usage:\n"
              << "  strategy_experiment_main --input <csv> --limit-price <price>\n"
              << "                          [--output <csv>] [--symbol <symbol>]\n"
              << "                          [--mode passive|aggressive]\n"
              << "                          [--side buy|sell]\n"
              << "                          [--quantity N]\n"
              << "                          [--entry-offset N]\n";
}

const char *ModeName(StrategyMode mode) {
    return mode == StrategyMode::Aggressive ? "aggressive" : "passive";
}

void PrintOptionalPrice(const char *label, const std::optional<double> &price) {
    if (price.has_value()) {
        std::cout << label << *price << '\n';
    } else {
        std::cout << label << "n/a\n";
    }
}

bool EnsureOutputDirectory(const std::string &output_csv) {
    const std::filesystem::path output_path(output_csv);
    const std::filesystem::path parent_path = output_path.parent_path();

    if (parent_path.empty()) {
        return true;
    }

    std::error_code error;
    std::filesystem::create_directories(parent_path, error);

    if (error) {
        std::cerr << "Failed to create output directory: " << parent_path.string() << '\n';
        return false;
    }

    return true;
}

} // namespace
} // namespace bookforge

int main(int argc, char **argv) {
    using namespace bookforge;

    try {
        CliOptions opts;
        if (!ParseArgs(argc, argv, opts)) {
            PrintUsage();
            return 1;
        }

        StrategyMode mode;
        if (!ParseMode(opts.mode, mode)) {
            std::cerr << "Invalid --mode value. Expected passive or aggressive.\n";
            return 1;
        }

        bool is_buy = false;
        if (!ParseSide(opts.side, is_buy)) {
            std::cerr << "Invalid --side value. Expected buy or sell.\n";
            return 1;
        }

        HyperliquidCsvReader reader(opts.input_csv);
        const std::vector<ExternalOrderEvent> input_events = reader.read_all(false, true);
        const std::vector<ExternalOrderEvent> replay_events =
            FilterReplayEventsBySymbol(input_events, opts.symbol, kFallbackSymbol);

        if (opts.entry_offset >= replay_events.size()) {
            std::cerr << "Invalid --entry-offset value. It must be smaller than the "
                      << "number of replayed events.\n";
            return 1;
        }

        ReplayConfig replay_config;
        replay_config.path = opts.input_csv;
        replay_config.symbol = opts.symbol.empty() ? kFallbackSymbol : opts.symbol;
        replay_config.source = ReplaySource::Hyperliquid;
        replay_config.max_events = 0;
        replay_config.start_offset = 0;
        replay_config.pacing_mode = ReplayPacingMode::Unpaced;
        replay_config.replay_speed = 1.0;
        replay_config.log_every_n = 500000;
        replay_config.log_summary = false;
        replay_config.log_errors = true;
        replay_config.strict_mode = false;

        StrategyExperimentConfig experiment_config;
        experiment_config.mode = mode;
        experiment_config.csv_path = opts.input_csv;
        experiment_config.entry_offset = opts.entry_offset;
        experiment_config.is_buy = is_buy;
        experiment_config.limit_price = opts.limit_price;
        experiment_config.quantity = opts.quantity;
        experiment_config.timing = InjectedOrderTiming::AfterEvent;

        StrategyExperimentRunner runner(replay_config);
        const StrategyExperimentResult result = runner.RunOnce(
            experiment_config, replay_events, "strategy-experiment-order", "strategy-experiment");

        if (!EnsureOutputDirectory(opts.output_csv)) {
            return 1;
        }

        if (!StrategyExperimentCsvWriter::Write(opts.output_csv, {result})) {
            std::cerr << "Failed to write strategy experiment results: " << opts.output_csv << '\n';
            return 1;
        }

        std::cout << "Strategy experiment configuration:\n"
                  << "Input: " << opts.input_csv << '\n';

        if (opts.symbol.empty()) {
            std::cout << "Symbol filter: all\n";
        } else {
            std::cout << "Symbol filter: " << opts.symbol << '\n';
        }

        std::cout << "Fallback symbol: " << kFallbackSymbol << '\n'
                  << "Input events: " << input_events.size() << '\n'
                  << "Replayed events: " << replay_events.size() << '\n'
                  << "Mode: " << ModeName(result.mode) << '\n'
                  << "Side: " << (result.is_buy ? "buy" : "sell") << '\n'
                  << "Limit price: " << result.limit_price << '\n'
                  << "Quantity: " << result.requested_qty << '\n'
                  << "Entry offset: " << result.entry_offset << '\n'
                  << "Filled quantity: " << result.filled_qty << '\n'
                  << "Remaining quantity: " << result.remaining_qty << '\n'
                  << "Fill rate: " << result.fill_rate << '\n'
                  << "Average execution price: " << result.avg_execution_price << '\n'
                  << "Implementation shortfall (bps): " << result.implementation_shortfall_bps
                  << '\n'
                  << "Output: " << opts.output_csv << '\n';

        PrintOptionalPrice("Decision best bid: ", result.decision_best_bid);
        PrintOptionalPrice("Decision best ask: ", result.decision_best_ask);

        if (result.has_decision_metrics) {
            std::cout << "Decision mid-price: " << result.decision_mid_price << '\n'
                      << "Decision spread: " << result.decision_spread << '\n';
        } else {
            std::cout << "Decision mid-price: n/a\n"
                      << "Decision spread: n/a\n";
        }

        return 0;
    } catch (const std::exception &exception) {
        std::cerr << "Strategy experiment failed: " << exception.what() << '\n';
        return 1;
    }
}
