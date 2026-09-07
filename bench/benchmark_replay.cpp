#include <benchmark/benchmark.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "HyperliquidCsvReader.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

using namespace bookforge;

namespace {

std::vector<ExternalOrderEvent> LoadFixtureEvents(const std::string &path) {
    HyperliquidCsvReader reader(path);
    std::vector<ExternalOrderEvent> events = reader.read_all(false, false);

    if (events.empty()) {
        throw std::runtime_error("Replay benchmark fixture produced zero events: " + path);
    }

    return events;
}

const std::vector<ExternalOrderEvent> &BenchmarkEvents() {
    static const std::vector<ExternalOrderEvent> events =
        LoadFixtureEvents(BOOKFORGE_BENCH_FIXTURE_FILE);
    return events;
}

std::int64_t CountEvents(const std::vector<ExternalOrderEvent> &events, EventType event_type) {
    std::int64_t count = 0;

    for (const ExternalOrderEvent &event : events) {
        if (event.eventType == event_type) {
            ++count;
        }
    }

    return count;
}

static void BM_InMemoryReplayThroughput(benchmark::State &state) {
    const std::vector<ExternalOrderEvent> &events = BenchmarkEvents();

    state.counters["events"] = static_cast<double>(events.size());
    state.counters["new_events"] = static_cast<double>(CountEvents(events, EventType::New));
    state.counters["reject_events"] = static_cast<double>(CountEvents(events, EventType::Reject));

    for (auto _ : state) {
        MatchingEngine engine;
        HyperliquidMatchingEngineAdapter adapter(engine);

        ReplayConfig config;
        config.start_offset = 0;
        config.max_events = static_cast<std::uint64_t>(events.size());
        config.log_every_n = 0;
        config.log_summary = false;
        config.log_errors = false;

        ReplayRunner runner(config);
        const bool replay_completed = runner.Run(adapter, events);

        benchmark::DoNotOptimize(replay_completed);
        benchmark::DoNotOptimize(engine);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) *
                            static_cast<std::int64_t>(events.size()));
}

BENCHMARK(BM_InMemoryReplayThroughput)->MinTime(2.0);

} // namespace
