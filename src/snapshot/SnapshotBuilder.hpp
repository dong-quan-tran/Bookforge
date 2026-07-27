#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/matching_engine.hpp"
#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

struct SnapshotBuildContext {
    std::string symbol;
    std::uint64_t replay_event_index{0};
    std::uint64_t replay_timestamp_ns{0};

    std::uint64_t total_events_seen{0};
    std::uint64_t submitted_orders{0};
    std::uint64_t rejected_events{0};
    std::uint64_t ignored_events{0};
    std::uint64_t generated_trades{0};
};

class SnapshotBuilder {
  public:
    [[nodiscard]] static BookSnapshot
    Build(const MatchingEngine &engine, const SnapshotBuildContext &ctx, std::size_t depth_levels);
};

} // namespace bookforge