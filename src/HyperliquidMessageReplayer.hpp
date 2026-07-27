#pragma once

#include "ExternalOrderEvent.hpp"
#include "HyperliquidCsvReader.hpp"

// Include your existing OrderBook / MatchingEngine headers
// #include "OrderBook.hpp"
// #include "MatchingEngine.hpp"

namespace bookforge {

class HyperliquidMessageReplayer {
  public:
    HyperliquidMessageReplayer(std::string csvPath
                               /*, OrderBook& book, MatchingEngine& engine */
                               )
        : reader_(std::move(csvPath)) {}

    void replay() {
        auto events = reader_.read_all();
        for (const auto &ev : events) {
            // TODO: call into your existing engine / book logic.
            // Example skeleton:
            //
            // switch (ev.eventType) {
            // case EventType::New:
            //     engine.place_limit_order(ev);
            //     break;
            // case EventType::Cancel:
            //     engine.cancel_order(ev);
            //     break;
            // case EventType::Fill:
            //     engine.apply_fill(ev);
            //     break;
            // case EventType::Reject:
            //     // Might skip or log only
            //     break;
            // case EventType::Trigger:
            //     // Could model trigger orders if you want
            //     break;
            // default:
            //     break;
            // }
        }
    }

  private:
    HyperliquidCsvReader reader_;
};

} // namespace bookforge