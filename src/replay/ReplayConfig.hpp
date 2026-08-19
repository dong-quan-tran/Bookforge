#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bookforge {

enum class ReplaySource { Hyperliquid, Lobster };

enum class ReplayPacingMode {
    Unpaced,
    EventTime,
};

struct ReplayConfig {
    // Input location
    std::string path;

    // Logical symbol descriptor (e.g., BTCUSDT.P, AAPL)
    std::string symbol;

    // Data source type (Hyperliquid enriched CSV, LOBSTER messages, etc.)
    ReplaySource source{ReplaySource::Hyperliquid};

    // Bounded replay controls for tests and experiments
    std::uint64_t max_events{0};   // 0 = no explicit limit
    std::uint64_t start_offset{0}; // skip first N events, 0 = from beginning

    // Replay pacing. Unpaced preserves current fastest-possible replay behavior.
    ReplayPacingMode pacing_mode{ReplayPacingMode::Unpaced};
    double replay_speed{1.0}; // EventTime mode multiplier; must be positive when used.

    // Logging / observability
    std::uint64_t log_every_n{0}; // 0 = no periodic progress logging
    bool log_summary{true};       // print summary at end
    bool log_errors{true};        // print malformed row diagnostics

    // Strictness / error handling
    bool strict_mode{false}; // true = stop on first malformed row

    // Optional filters for advanced use
    std::vector<std::string> participant_filters;
    std::vector<std::string> side_filters; // e.g., {"buy", "sell"}
};

} // namespace bookforge