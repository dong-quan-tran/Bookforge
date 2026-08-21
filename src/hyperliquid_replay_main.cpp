#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "HyperliquidCsvReader.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

using namespace bookforge;

namespace {

void PrintUsage(const char *program_name) {
    std::cout << "Usage:\n"
              << "  " << program_name
              << " [input_csv] [--pacing unpaced|event-time] [--speed <positive-number>]\n";
}

bool ParsePacingMode(const std::string &value, ReplayPacingMode &pacing_mode) {
    if (value == "unpaced") {
        pacing_mode = ReplayPacingMode::Unpaced;
        return true;
    }

    if (value == "event-time") {
        pacing_mode = ReplayPacingMode::EventTime;
        return true;
    }

    return false;
}

const char *PacingModeName(ReplayPacingMode pacing_mode) {
    switch (pacing_mode) {
    case ReplayPacingMode::Unpaced:
        return "unpaced";
    case ReplayPacingMode::EventTime:
        return "event-time";
    }

    return "unknown";
}

bool ParseArgs(int argc, char **argv, ReplayConfig &config) {
    bool input_path_seen = false;

    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);

        if (argument == "--help" || argument == "-h") {
            PrintUsage(argv[0]);
            return false;
        }

        if (argument == "--pacing") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --pacing.\n";
                return false;
            }

            if (!ParsePacingMode(argv[++i], config.pacing_mode)) {
                std::cerr << "Invalid --pacing value. Expected unpaced or event-time.\n";
                return false;
            }

            continue;
        }

        if (argument == "--speed") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --speed.\n";
                return false;
            }

            try {
                config.replay_speed = std::stod(argv[++i]);
            } catch (const std::exception &) {
                std::cerr << "Invalid --speed value. Expected a positive number.\n";
                return false;
            }

            if (config.replay_speed <= 0.0) {
                std::cerr << "Invalid --speed value. Expected a positive number.\n";
                return false;
            }

            continue;
        }

        if (!argument.empty() && argument.front() == '-') {
            std::cerr << "Unknown option: " << argument << '\n';
            return false;
        }

        if (input_path_seen) {
            std::cerr << "Only one input CSV path may be provided.\n";
            return false;
        }

        config.path = argument;
        input_path_seen = true;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    try {
        ReplayConfig config;
        config.path = "data/processed/hyperliquid_sample.csv";
        config.symbol = "BTCUSDT.P";
        config.source = ReplaySource::Hyperliquid;
        config.max_events = 0;
        config.start_offset = 0;
        config.pacing_mode = ReplayPacingMode::Unpaced;
        config.replay_speed = 1.0;
        config.log_every_n = 500000;
        config.log_summary = true;
        config.log_errors = true;
        config.strict_mode = false;

        if (!ParseArgs(argc, argv, config)) {
            PrintUsage(argv[0]);
            return 1;
        }

        HyperliquidCsvReader reader(config.path);
        const auto events = reader.read_all(config.strict_mode, config.log_errors);

        MatchingEngine engine;
        HyperliquidMatchingEngineAdapter adapter(engine);

        ReplayRunner runner(config);
        if (!runner.Run(adapter, events)) {
            std::cerr << "Replay runner failed.\n";
            return 1;
        }

        const auto &stats = adapter.Stats();
        std::cout << "Replay configuration:\n"
                  << "Input: " << config.path << "\n"
                  << "Pacing: " << PacingModeName(config.pacing_mode) << "\n"
                  << "Speed: " << config.replay_speed << "x\n"
                  << "Total events: " << stats.totalEvents << "\n"
                  << "New: " << stats.newCount << "\n"
                  << "Cancel: " << stats.cancelCount << "\n"
                  << "Fill: " << stats.fillCount << "\n"
                  << "Reject: " << stats.rejectCount << "\n"
                  << "Trigger: " << stats.triggerCount << "\n"
                  << "Other: " << stats.otherCount << "\n"
                  << "Submitted orders: " << stats.submittedOrders << "\n"
                  << "Ignored events: " << stats.ignoredEvents << "\n"
                  << "Generated trades: " << stats.generatedTrades << "\n";

        const auto best_bid = engine.Book().GetBestBid();
        const auto best_ask = engine.Book().GetBestAsk();
        const auto mid = engine.Book().GetMidPrice();
        const auto spread = engine.Book().GetSpread();

        if (best_bid.has_value()) {
            std::cout << "Final best bid: " << *best_bid << "\n";
        } else {
            std::cout << "Final best bid: n/a\n";
        }

        if (best_ask.has_value()) {
            std::cout << "Final best ask: " << *best_ask << "\n";
        } else {
            std::cout << "Final best ask: n/a\n";
        }

        if (mid.has_value()) {
            std::cout << "Final mid-price: " << *mid << "\n";
        } else {
            std::cout << "Final mid-price: n/a\n";
        }

        if (spread.has_value()) {
            std::cout << "Final spread: " << *spread << "\n";
        } else {
            std::cout << "Final spread: n/a\n";
        }

        return 0;
    } catch (const std::exception &exception) {
        std::cerr << "Replay failed: " << exception.what() << "\n";
        return 1;
    }
}