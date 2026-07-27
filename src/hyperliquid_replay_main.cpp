#include <exception>
#include <iostream>

#include "HyperliquidCsvReader.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

using namespace bookforge;

int main(int argc, char **argv) {
    try {
        ReplayConfig config;
        config.path = (argc > 1) ? argv[1] : "data/processed/hyperliquid_sample.csv";
        config.symbol = "BTCUSDT.P";
        config.source = ReplaySource::Hyperliquid;
        config.max_events = 0; // 0 = full file
        config.start_offset = 0;
        config.log_every_n = 500000;
        config.log_summary = true;
        config.log_errors = true;
        config.strict_mode = false;

        HyperliquidCsvReader reader(config.path);
        auto events = reader.read_all(config.strict_mode, config.log_errors);

        MatchingEngine engine;
        HyperliquidMatchingEngineAdapter adapter(engine);

        ReplayRunner runner(config);
        runner.Run(adapter, events);

        const auto &stats = adapter.Stats();
        std::cout << "Total events: " << stats.totalEvents << "\n"
                  << "New: " << stats.newCount << "\n"
                  << "Cancel: " << stats.cancelCount << "\n"
                  << "Fill: " << stats.fillCount << "\n"
                  << "Reject: " << stats.rejectCount << "\n"
                  << "Trigger: " << stats.triggerCount << "\n"
                  << "Other: " << stats.otherCount << "\n"
                  << "Submitted orders: " << stats.submittedOrders << "\n"
                  << "Ignored events: " << stats.ignoredEvents << "\n"
                  << "Generated trades: " << stats.generatedTrades << "\n";

        auto best_bid = engine.Book().GetBestBid();
        auto best_ask = engine.Book().GetBestAsk();
        auto mid = engine.Book().GetMidPrice();
        auto spread = engine.Book().GetSpread();

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
    } catch (const std::exception &ex) {
        std::cerr << "Replay failed: " << ex.what() << "\n";
        return 1;
    }
}