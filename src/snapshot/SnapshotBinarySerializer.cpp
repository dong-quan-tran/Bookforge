#include "snapshot/SnapshotBinarySerializer.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace bookforge {
namespace {

constexpr char kMagic[8] = {'B', 'F', 'S', 'N', 'A', 'P', '0', '1'};
constexpr std::uint32_t kVersion = 1;

void WriteBytes(std::ofstream &out, const void *data, std::size_t size) {
    out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
    if (!out) {
        throw std::runtime_error("failed to write binary snapshot bytes");
    }
}

void WriteU8(std::ofstream &out, std::uint8_t value) {
    WriteBytes(out, &value, sizeof(value));
}

void WriteU32(std::ofstream &out, std::uint32_t value) {
    std::uint8_t buf[4] = {static_cast<std::uint8_t>(value & 0xffu),
                           static_cast<std::uint8_t>((value >> 8) & 0xffu),
                           static_cast<std::uint8_t>((value >> 16) & 0xffu),
                           static_cast<std::uint8_t>((value >> 24) & 0xffu)};
    WriteBytes(out, buf, sizeof(buf));
}

void WriteU64(std::ofstream &out, std::uint64_t value) {
    std::uint8_t buf[8] = {static_cast<std::uint8_t>(value & 0xffu),
                           static_cast<std::uint8_t>((value >> 8) & 0xffu),
                           static_cast<std::uint8_t>((value >> 16) & 0xffu),
                           static_cast<std::uint8_t>((value >> 24) & 0xffu),
                           static_cast<std::uint8_t>((value >> 32) & 0xffu),
                           static_cast<std::uint8_t>((value >> 40) & 0xffu),
                           static_cast<std::uint8_t>((value >> 48) & 0xffu),
                           static_cast<std::uint8_t>((value >> 56) & 0xffu)};
    WriteBytes(out, buf, sizeof(buf));
}

void WriteDouble(std::ofstream &out, double value) {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    WriteU64(out, bits);
}

void WriteString(std::ofstream &out, std::string_view value) {
    WriteU32(out, static_cast<std::uint32_t>(value.size()));
    if (!value.empty()) {
        WriteBytes(out, value.data(), value.size());
    }
}

void WriteOptionalDouble(std::ofstream &out, const std::optional<double> &value) {
    WriteU8(out, value.has_value() ? 1u : 0u);
    if (value.has_value()) {
        WriteDouble(out, *value);
    }
}

void WriteDepthSide(std::ofstream &out, const std::vector<DepthLevelSnapshot> &side,
                    std::size_t depth_levels) {
    if (side.size() > depth_levels) {
        throw std::runtime_error("snapshot depth exceeds configured depth_levels");
    }

    WriteU32(out, static_cast<std::uint32_t>(side.size()));
    for (const auto &level : side) {
        WriteDouble(out, level.price);
        WriteU32(out, level.quantity);
    }
}

} // namespace

void SnapshotBinarySerializer::Write(const std::string &path,
                                     const std::vector<BookSnapshot> &snapshots,
                                     std::size_t depth_levels) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("failed to open binary snapshot file for writing: " + path);
    }

    WriteBytes(out, kMagic, sizeof(kMagic));
    WriteU32(out, kVersion);
    WriteU32(out, 0u);
    WriteU32(out, static_cast<std::uint32_t>(depth_levels));
    WriteU64(out, static_cast<std::uint64_t>(snapshots.size()));

    for (const auto &snapshot : snapshots) {
        WriteString(out, snapshot.symbol);

        WriteU64(out, snapshot.replay_event_index);
        WriteU64(out, snapshot.replay_timestamp_ns);
        WriteU64(out, snapshot.total_events_seen);
        WriteU64(out, snapshot.submitted_orders);
        WriteU64(out, snapshot.rejected_events);
        WriteU64(out, snapshot.ignored_events);
        WriteU64(out, snapshot.generated_trades);

        WriteOptionalDouble(out, snapshot.best_bid);
        WriteOptionalDouble(out, snapshot.best_ask);
        WriteOptionalDouble(out, snapshot.mid_price);
        WriteOptionalDouble(out, snapshot.spread);

        WriteDepthSide(out, snapshot.bids, depth_levels);
        WriteDepthSide(out, snapshot.asks, depth_levels);
    }
}

} // namespace bookforge