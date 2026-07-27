#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "HyperliquidCsvReader.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "features/FeatureBuilder.hpp"
#include "features/FeatureCsvWriter.hpp"
#include "features/OfiFeatureBuilder.hpp"
#include "features/RollingFeatureBuilder.hpp"
#include "snapshot/SnapshotBuilder.hpp"

using namespace bookforge;

namespace {

struct ExportConfig {
    std::string input_path = "data/processed/hyperliquid_sample.csv";
    std::string output_path = "output/features.csv";
    std::string symbol = "BTCUSDT.P";
    std::size_t snapshot_depth = 10;
    std::size_t imbalance_depth = 10;
    std::size_t ofi_depth = 10;
    std::size_t rolling_window = 50;
    std::size_t max_events = 0;
    std::size_t start_offset = 0;
    std::size_t log_every_n = 500000;
    bool strict_mode = false;
    bool log_errors = true;
};

void PrintUsage(const char *program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  --input PATH              Input Hyperliquid CSV path\n"
              << "  --output PATH             Output feature CSV path\n"
              << "  --symbol SYMBOL           Symbol name to stamp into snapshots/features\n"
              << "  --snapshot-depth N        Depth levels captured in snapshots\n"
              << "  --imbalance-depth N       Depth levels used for depth imbalance features\n"
              << "  --ofi-depth N             Depth levels used for OFI features\n"
              << "  --rolling-window N        Rolling window size in rows\n"
              << "  --max-events N            Maximum events to process (0 = all)\n"
              << "  --start-offset N          Event offset to start from\n"
              << "  --log-every N             Progress log frequency\n"
              << "  --strict                  Enable strict CSV parsing mode\n"
              << "  --no-log-errors           Disable CSV parse error logging\n"
              << "  --help                    Show this help text\n";
}

ExportConfig ParseArgs(int argc, char **argv) {
    ExportConfig cfg;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto require_value = [&](const std::string &name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing value for argument: " + name);
            }
            return argv[++i];
        };

        if (arg == "--input") {
            cfg.input_path = require_value(arg);
        } else if (arg == "--output") {
            cfg.output_path = require_value(arg);
        } else if (arg == "--symbol") {
            cfg.symbol = require_value(arg);
        } else if (arg == "--snapshot-depth") {
            cfg.snapshot_depth = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--imbalance-depth") {
            cfg.imbalance_depth = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--ofi-depth") {
            cfg.ofi_depth = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--rolling-window") {
            cfg.rolling_window = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--max-events") {
            cfg.max_events = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--start-offset") {
            cfg.start_offset = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--log-every") {
            cfg.log_every_n = static_cast<std::size_t>(std::stoull(require_value(arg)));
        } else if (arg == "--strict") {
            cfg.strict_mode = true;
        } else if (arg == "--no-log-errors") {
            cfg.log_errors = false;
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }

    return cfg;
}

} // namespace

int main(int argc, char **argv) {
    try {
        const ExportConfig cfg = ParseArgs(argc, argv);

        HyperliquidCsvReader reader(cfg.input_path);
        const auto events = reader.read_all(cfg.strict_mode, cfg.log_errors);

        MatchingEngine engine;
        HyperliquidMatchingEngineAdapter adapter(engine);

        std::vector<BookSnapshot> snapshots;
        snapshots.reserve(events.size());

        const std::size_t total = events.size();
        const std::size_t start = cfg.start_offset > total ? total : cfg.start_offset;

        std::size_t processed = 0;

        for (std::size_t i = start; i < total; ++i) {
            if (cfg.max_events != 0 && processed >= cfg.max_events) {
                break;
            }

            const auto &ev = events[i];
            adapter.OnEvent(ev);
            ++processed;

            SnapshotBuildContext ctx;
            ctx.symbol = cfg.symbol;
            ctx.replay_event_index = static_cast<std::uint64_t>(processed);
            ctx.replay_timestamp_ns = static_cast<std::uint64_t>(ev.ts.count());

            const auto &stats = adapter.Stats();
            ctx.total_events_seen = static_cast<std::uint64_t>(stats.totalEvents);
            ctx.submitted_orders = static_cast<std::uint64_t>(stats.submittedOrders);
            ctx.rejected_events = static_cast<std::uint64_t>(stats.rejectCount);
            ctx.ignored_events = static_cast<std::uint64_t>(stats.ignoredEvents);
            ctx.generated_trades = static_cast<std::uint64_t>(stats.generatedTrades);

            snapshots.push_back(SnapshotBuilder::Build(engine, ctx, cfg.snapshot_depth));

            if (cfg.log_every_n != 0 && processed % cfg.log_every_n == 0) {
                std::cout << "[feature_export] processed=" << processed << '\n';
            }
        }

        auto rows = FeatureBuilder::BuildFromSnapshots(snapshots, cfg.imbalance_depth);

        OfiFeatureBuilder::AddOfiFeatures(rows, snapshots, cfg.ofi_depth);

        RollingFeatureBuilder::AddRollingContextFeatures(rows, cfg.rolling_window);

        const std::filesystem::path output_path(cfg.output_path);
        if (output_path.has_parent_path()) {
            std::filesystem::create_directories(output_path.parent_path());
        }

        FeatureCsvWriter::Write(cfg.output_path, rows);

        std::cout << "[feature_export] wrote_rows=" << rows.size() << "\n"
                  << "[feature_export] output=" << cfg.output_path << "\n";

        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "[feature_export] failed: " << ex.what() << "\n";
        return 1;
    }
}