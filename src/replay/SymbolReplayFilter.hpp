#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ExternalOrderEvent.hpp"

namespace bookforge {

inline bool MatchesReplaySymbol(const ExternalOrderEvent &event, const std::string &selected_symbol,
                                const std::string &fallback_symbol) {
    if (selected_symbol.empty()) {
        return true;
    }

    if (!event.symbol.empty()) {
        return event.symbol == selected_symbol;
    }

    return fallback_symbol == selected_symbol;
}

inline std::vector<ExternalOrderEvent>
FilterReplayEventsBySymbol(const std::vector<ExternalOrderEvent> &events,
                           const std::string &selected_symbol, const std::string &fallback_symbol) {
    if (selected_symbol.empty()) {
        return events;
    }

    std::vector<ExternalOrderEvent> filtered_events;
    filtered_events.reserve(events.size());

    for (const ExternalOrderEvent &event : events) {
        if (MatchesReplaySymbol(event, selected_symbol, fallback_symbol)) {
            filtered_events.push_back(event);
        }
    }

    return filtered_events;
}

} // namespace bookforge
