#include "snapshot/SnapshotBinaryDeserializer.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <vector>

namespace bookforge {
namespace {

constexpr char kMagic[8] = {'B', 'F', 'S', 'N', 'A', 'P', '0', '1'};
constexpr std::uint32_t kVersion = 1;

void ReadBytes(std::ifstream &in, void *data, std::size_t size) {
    in.read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(size));
    if (!in) {
        throw std::runtime_error("failed to read binary snapshot bytes");
    }
}

std::uint8_t ReadU8(std::ifstream &in) {
    std::uint8_t value = 0;
    ReadBytes(in, &value, sizeof(value));
    return value;
}

std::uint32_t ReadU32(std::ifstream &in) {
    std::uint8_t buf[4];
    ReadBytes(in, buf, sizeof(buf));
    return static_cast<std::uint32_t>(buf[0]) | (static_cast<std::uint32_t>(buf[1]) << 8) |
           (static_cast<std::uint32_t>(buf[2]) << 16) | (static_cast<std::uint32_t>(buf[3]) << 24);
}

std::uint64_t ReadU64(std::ifstream &in) {
    std::uint8_t buf[8];
    ReadBytes(in, buf, sizeof(buf));
    return static_cast<std::uint64_t>(buf[0]) | (static_cast<std::uint64_t>(buf[1]) << 8) |
           (static_cast<std::uint64_t>(buf[2]) << 16) | (static_cast<std::uint64_t>(buf[3]) << 24) |
           (static_cast<std::uint64_t>(buf[4]) << 32) | (static_cast<std::uint64_t>(buf[5]) << 40) |
           (static_cast<std::uint64_t>(buf[6]) << 48) | (static_cast<std::uint64_t>(buf[7]) << 56);
}

double ReadDouble(std::ifstream &in) {
    const std::uint64_t bits = ReadU64(in);
    double value = 0.0;
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::string ReadString(std::ifstream &in) {
    const std::uint32_t len = ReadU32(in);
    std::string value(len, '\0');
    if (len > 0) {
        ReadBytes(in, value.data(), len);
    }
    return value;
}

std::optional<double> ReadOptionalDouble(std::ifstream &in) {
    const std::uint8_t present = ReadU8(in);
    if (present > 1u) {
        throw std::runtime_error("invalid optional-double presence flag");
    }
    if (present == 0u) {
        return std::nullopt;
    }
    return ReadDouble(in);
}

std::vector<DepthLevelSnapshot> ReadDepthSide(std::ifstream &in,
                                              std::size_t expected_depth_levels) {
    const std::uint32_t count = ReadU32(in);
    if (count > expected_depth_levels) {
        throw std::runtime_error("binary snapshot depth exceeds expected_depth_levels");
    }

    std::vector<DepthLevelSnapshot> side;
    side.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        side.push_back(DepthLevelSnapshot{ReadDouble(in), ReadU32(in)});
    }
    return side;
}

} // namespace

std::vector<BookSnapshot> SnapshotBinaryDeserializer::Read(const std::string &path,
                                                           std::size_t expected_depth_levels) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("failed to open binary snapshot file for reading: " + path);
    }

    char magic[8];
    ReadBytes(in, magic, sizeof(magic));
    if (std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
        throw std::runtime_error("invalid binary snapshot magic");
    }

    const std::uint32_t version = ReadU32(in);
    if (version != kVersion) {
        throw std::runtime_error("unsupported binary snapshot version");
    }

    const std::uint32_t reserved = ReadU32(in);
    if (reserved != 0u) {
        throw std::runtime_error("unexpected nonzero reserved field in binary snapshot header");
    }

    const std::uint32_t depth_levels = ReadU32(in);
    if (depth_levels != expected_depth_levels) {
        throw std::runtime_error("binary snapshot depth_levels mismatch");
    }

    const std::uint64_t snapshot_count = ReadU64(in);
    std::vector<BookSnapshot> snapshots;
    snapshots.reserve(static_cast<std::size_t>(snapshot_count));

    for (std::uint64_t i = 0; i < snapshot_count; ++i) {
        BookSnapshot snapshot{};
        snapshot.symbol = ReadString(in);

        snapshot.replay_event_index = ReadU64(in);
        snapshot.replay_timestamp_ns = ReadU64(in);
        snapshot.total_events_seen = ReadU64(in);
        snapshot.submitted_orders = ReadU64(in);
        snapshot.rejected_events = ReadU64(in);
        snapshot.ignored_events = ReadU64(in);
        snapshot.generated_trades = ReadU64(in);

        snapshot.best_bid = ReadOptionalDouble(in);
        snapshot.best_ask = ReadOptionalDouble(in);
        snapshot.mid_price = ReadOptionalDouble(in);
        snapshot.spread = ReadOptionalDouble(in);

        snapshot.bids = ReadDepthSide(in, expected_depth_levels);
        snapshot.asks = ReadDepthSide(in, expected_depth_levels);

        snapshots.push_back(std::move(snapshot));
    }

    return snapshots;
}

} // namespace bookforge