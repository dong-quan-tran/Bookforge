#include "snapshot/SnapshotDeserializer.hpp"

#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace bookforge {
namespace {

std::optional<double> ParseOptionalDouble(const std::string &s) {
    if (s.empty()) {
        return std::nullopt;
    }
    return std::stod(s);
}

std::uint64_t ParseUint64(const std::string &s, const std::string &field_name) {
    if (s.empty()) {
        throw std::runtime_error("missing uint64 field: " + field_name);
    }
    return static_cast<std::uint64_t>(std::stoull(s));
}

std::uint32_t ParseUint32(const std::string &s, const std::string &field_name) {
    if (s.empty()) {
        throw std::runtime_error("missing uint32 field: " + field_name);
    }
    return static_cast<std::uint32_t>(std::stoul(s));
}

} // namespace

std::vector<BookSnapshot> SnapshotDeserializer::ReadCsv(const std::string &path,
                                                        std::size_t depth_levels) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("failed to open snapshot CSV for reading: " + path);
    }

    std::string header_line;
    if (!std::getline(in, header_line)) {
        throw std::runtime_error("snapshot CSV is empty: " + path);
    }

    ValidateHeader(SplitCsvLine(header_line), depth_levels);

    std::vector<BookSnapshot> snapshots;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        snapshots.push_back(ParseRow(SplitCsvLine(line), depth_levels));
    }

    return snapshots;
}

std::vector<std::string> SnapshotDeserializer::SplitCsvLine(const std::string &line) {
    std::vector<std::string> cells;
    std::stringstream ss(line);
    std::string cell;

    while (std::getline(ss, cell, ',')) {
        cells.push_back(cell);
    }

    if (!line.empty() && line.back() == ',') {
        cells.push_back("");
    }

    return cells;
}

std::vector<std::string> SnapshotDeserializer::ExpectedHeader(std::size_t depth_levels) {
    std::vector<std::string> header = {
        "symbol",           "replay_event_index", "replay_timestamp_ns", "total_events_seen",
        "submitted_orders", "rejected_events",    "ignored_events",      "generated_trades",
        "best_bid",         "best_ask",           "mid_price",           "spread"};

    for (std::size_t i = 1; i <= depth_levels; ++i) {
        header.push_back("bid_px_" + std::to_string(i));
        header.push_back("bid_qty_" + std::to_string(i));
    }

    for (std::size_t i = 1; i <= depth_levels; ++i) {
        header.push_back("ask_px_" + std::to_string(i));
        header.push_back("ask_qty_" + std::to_string(i));
    }

    return header;
}

void SnapshotDeserializer::ValidateHeader(const std::vector<std::string> &actual,
                                          std::size_t depth_levels) {
    const auto expected = ExpectedHeader(depth_levels);
    if (actual.size() != expected.size()) {
        throw std::runtime_error("snapshot CSV header column count mismatch");
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (actual[i] != expected[i]) {
            throw std::runtime_error("snapshot CSV header mismatch at column " + std::to_string(i));
        }
    }
}

BookSnapshot SnapshotDeserializer::ParseRow(const std::vector<std::string> &cells,
                                            std::size_t depth_levels) {
    const std::size_t expected_cells = 12 + (2 * depth_levels) + (2 * depth_levels);
    if (cells.size() != expected_cells) {
        throw std::runtime_error("snapshot CSV row column count mismatch");
    }

    BookSnapshot snapshot{};
    std::size_t i = 0;

    snapshot.symbol = cells[i++];
    snapshot.replay_event_index = ParseUint64(cells[i++], "replay_event_index");
    snapshot.replay_timestamp_ns = ParseUint64(cells[i++], "replay_timestamp_ns");
    snapshot.total_events_seen = ParseUint64(cells[i++], "total_events_seen");
    snapshot.submitted_orders = ParseUint64(cells[i++], "submitted_orders");
    snapshot.rejected_events = ParseUint64(cells[i++], "rejected_events");
    snapshot.ignored_events = ParseUint64(cells[i++], "ignored_events");
    snapshot.generated_trades = ParseUint64(cells[i++], "generated_trades");
    snapshot.best_bid = ParseOptionalDouble(cells[i++]);
    snapshot.best_ask = ParseOptionalDouble(cells[i++]);
    snapshot.mid_price = ParseOptionalDouble(cells[i++]);
    snapshot.spread = ParseOptionalDouble(cells[i++]);

    for (std::size_t level = 0; level < depth_levels; ++level) {
        const std::string &px = cells[i++];
        const std::string &qty = cells[i++];

        if (!px.empty() || !qty.empty()) {
            if (px.empty() || qty.empty()) {
                throw std::runtime_error("incomplete bid depth cell pair");
            }
            snapshot.bids.push_back(DepthLevelSnapshot{std::stod(px), ParseUint32(qty, "bid_qty")});
        }
    }

    for (std::size_t level = 0; level < depth_levels; ++level) {
        const std::string &px = cells[i++];
        const std::string &qty = cells[i++];

        if (!px.empty() || !qty.empty()) {
            if (px.empty() || qty.empty()) {
                throw std::runtime_error("incomplete ask depth cell pair");
            }
            snapshot.asks.push_back(DepthLevelSnapshot{std::stod(px), ParseUint32(qty, "ask_qty")});
        }
    }

    return snapshot;
}

} // namespace bookforge