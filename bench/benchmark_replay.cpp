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
    auto events = reader.read_all(false, false);

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

static void BM_ReplayThroughput(benchmark::State &state) {
    const auto &events = BenchmarkEvents();

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
        const bool ok = runner.Run(adapter, events);

        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(engine);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(events.size()));
}

BENCHMARK(BM_ReplayThroughput)->MinTime(0.5);

} // namespace