#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "IReplayAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/ReplayClock.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

using namespace bookforge;

namespace {

ExternalOrderEvent MakeEvent(EventType type, bool is_ask, double price, double size,
                             int status_id = 1, const std::string &status_text = "open",
                             std::int64_t ts_ns = 0) {
    ExternalOrderEvent event{};
    event.ts = std::chrono::nanoseconds{ts_ns};
    event.price = price;
    event.size = size;
    event.isAsk = is_ask;
    event.statusId = status_id;
    event.statusText = status_text;
    event.eventType = type;
    return event;
}

ReplayConfig MakeQuietReplayConfig() {
    ReplayConfig config;
    config.log_every_n = 0;
    config.log_summary = false;
    config.log_errors = false;
    config.strict_mode = false;
    return config;
}

class RecordingReplayAdapter final : public IReplayAdapter {
  public:
    void OnEvent(const ExternalOrderEvent &) override {
        calls.push_back("event");
    }

    void OnInjectedOrder(const InjectedOrder &order) override {
        calls.push_back("inject:" + order.order_id);
    }

    const AdapterMetrics &Metrics() const override {
        return metrics_;
    }

    AdapterMetrics metrics_{};
    std::vector<std::string> calls;
};

} // namespace

TEST(ReplayConfigTest, DefaultsToUnpacedReplayWithUnitSpeed) {
    const ReplayConfig config;

    EXPECT_EQ(config.pacing_mode, ReplayPacingMode::Unpaced);
    EXPECT_DOUBLE_EQ(config.replay_speed, 1.0);
}

TEST(ReplayConfigTest, SupportsEventTimePacingConfiguration) {
    ReplayConfig config;
    config.pacing_mode = ReplayPacingMode::EventTime;
    config.replay_speed = 4.0;

    EXPECT_EQ(config.pacing_mode, ReplayPacingMode::EventTime);
    EXPECT_DOUBLE_EQ(config.replay_speed, 4.0);
}

TEST(ReplayRunnerTest, ProcessesOnlyBoundedPrefixWhenMaxEventsSet) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::Reject, false, 101.00, 0.02000, 3, "perpMarginRejected", 2),
        MakeEvent(EventType::New, true, 102.00, 0.03000, 1, "open", 3),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.max_events = 2;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2U);
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.rejectCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 1U);
    EXPECT_EQ(stats.ignoredEvents, 1U);

    const auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(ReplayRunnerTest, AppliesStartOffsetBeforeMaxEvents) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 2),
        MakeEvent(EventType::Reject, false, 101.00, 0.02000, 3, "perpMarginRejected", 3),
        MakeEvent(EventType::New, false, 99.75, 0.02000, 1, "open", 4),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.start_offset = 1;
    config.max_events = 2;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 2U);
    EXPECT_EQ(stats.newCount, 1U);
    EXPECT_EQ(stats.rejectCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 1U);
    EXPECT_EQ(stats.ignoredEvents, 1U);

    const auto best_ask = engine.Book().GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 100.50);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
}

TEST(ReplayRunnerTest, PreservesInputOrderingExactly) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 10),
        MakeEvent(EventType::New, false, 101.00, 0.01000, 1, "open", 20),
        MakeEvent(EventType::New, false, 99.50, 0.02000, 1, "open", 30),
    };

    ReplayConfig config = MakeQuietReplayConfig();

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 3U);
    EXPECT_EQ(stats.newCount, 3U);
    EXPECT_EQ(stats.generatedTrades, 1U);

    const auto &trades = adapter.Trades();
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.50);
    EXPECT_EQ(trades[0].side, Side::Buy);

    const auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 99.50);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(ReplayRunnerTest, OffsetPastEndProcessesNothing) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 2),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.start_offset = 10;
    config.max_events = 5;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 0U);
    EXPECT_EQ(stats.newCount, 0U);
    EXPECT_EQ(stats.submittedOrders, 0U);
    EXPECT_EQ(stats.generatedTrades, 0U);

    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(ReplayRunnerTest, MaxEventsZeroProcessesAllEvents) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 2),
        MakeEvent(EventType::Reject, false, 101.00, 0.02000, 3, "perpMarginRejected", 3),
    };

    ReplayConfig config = MakeQuietReplayConfig();

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 3U);
    EXPECT_EQ(stats.newCount, 2U);
    EXPECT_EQ(stats.rejectCount, 1U);
}

TEST(ReplayRunnerTest, EmptyInputSucceedsAndProcessesNothing) {
    const std::vector<ExternalOrderEvent> events;

    ReplayConfig config = MakeQuietReplayConfig();

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 0U);
    EXPECT_EQ(stats.newCount, 0U);
    EXPECT_EQ(stats.generatedTrades, 0U);
}

TEST(ReplayRunnerTest, StartOffsetAtEndProcessesNothing) {
    std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.00, 0.01000, 1, "open", 1),
        MakeEvent(EventType::New, true, 100.50, 0.01000, 1, "open", 2),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.start_offset = events.size();
    config.max_events = 10;

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);
    ReplayRunner runner(config);

    EXPECT_TRUE(runner.Run(adapter, events));

    const auto &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 0U);
    EXPECT_EQ(stats.submittedOrders, 0U);
}

TEST(ReplayRunnerPacingTest, EventTimePacingRequestsScaledTimestampDeltas) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.0, 0.01, 1, "open", 100),
        MakeEvent(EventType::New, false, 99.0, 0.01, 1, "open", 200),
        MakeEvent(EventType::New, true, 101.0, 0.01, 1, "open", 500),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.pacing_mode = ReplayPacingMode::EventTime;
    config.replay_speed = 2.0;

    RecordingReplayClock clock;
    RecordingReplayAdapter adapter;
    ReplayRunner runner(config, clock);

    ASSERT_TRUE(runner.Run(adapter, events));

    const auto &requested_sleeps = clock.RequestedSleeps();
    ASSERT_EQ(requested_sleeps.size(), 2U);
    EXPECT_EQ(requested_sleeps[0], std::chrono::nanoseconds{50});
    EXPECT_EQ(requested_sleeps[1], std::chrono::nanoseconds{150});
}

TEST(ReplayRunnerPacingTest, UnpacedReplayDoesNotRequestSleeps) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.0, 0.01, 1, "open", 100),
        MakeEvent(EventType::New, false, 99.0, 0.01, 1, "open", 200),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.pacing_mode = ReplayPacingMode::Unpaced;

    RecordingReplayClock clock;
    RecordingReplayAdapter adapter;
    ReplayRunner runner(config, clock);

    ASSERT_TRUE(runner.Run(adapter, events));

    EXPECT_TRUE(clock.RequestedSleeps().empty());
    EXPECT_TRUE(runner.Metrics().requested_pacing_delays.Empty());
}

TEST(ReplayRunnerPacingTest, NonMonotonicTimestampsDoNotRequestNegativeSleeps) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.0, 0.01, 1, "open", 500),
        MakeEvent(EventType::New, false, 99.0, 0.01, 1, "open", 400),
        MakeEvent(EventType::New, true, 101.0, 0.01, 1, "open", 600),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.pacing_mode = ReplayPacingMode::EventTime;
    config.replay_speed = 1.0;

    RecordingReplayClock clock;
    RecordingReplayAdapter adapter;
    ReplayRunner runner(config, clock);

    ASSERT_TRUE(runner.Run(adapter, events));

    const auto &requested_sleeps = clock.RequestedSleeps();
    ASSERT_EQ(requested_sleeps.size(), 1U);
    EXPECT_EQ(requested_sleeps[0], std::chrono::nanoseconds{200});

    const auto &histogram = runner.Metrics().requested_pacing_delays;
    EXPECT_EQ(histogram.Count(), 1U);
    EXPECT_EQ(histogram.Total(), std::chrono::nanoseconds{200});
}

TEST(ReplayRunnerPacingTest, NonPositiveReplaySpeedDoesNotRequestSleeps) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.0, 0.01, 1, "open", 100),
        MakeEvent(EventType::New, false, 99.0, 0.01, 1, "open", 200),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.pacing_mode = ReplayPacingMode::EventTime;
    config.replay_speed = 0.0;

    RecordingReplayClock clock;
    RecordingReplayAdapter adapter;
    ReplayRunner runner(config, clock);

    ASSERT_TRUE(runner.Run(adapter, events));

    EXPECT_TRUE(clock.RequestedSleeps().empty());
    EXPECT_TRUE(runner.Metrics().requested_pacing_delays.Empty());
}

TEST(ReplayRunnerPacingTest, RecordsRequestedDelaysInHistogramBuckets) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.0, 0.01, 1, "open", 0),
        MakeEvent(EventType::New, false, 99.0, 0.01, 1, "open", 500),
        MakeEvent(EventType::New, true, 101.0, 0.01, 1, "open", 5500),
        MakeEvent(EventType::New, true, 102.0, 0.01, 1, "open", 105500),
        MakeEvent(EventType::New, true, 103.0, 0.01, 1, "open", 2105500),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.pacing_mode = ReplayPacingMode::EventTime;
    config.replay_speed = 1.0;

    RecordingReplayClock clock;
    RecordingReplayAdapter adapter;
    ReplayRunner runner(config, clock);

    ASSERT_TRUE(runner.Run(adapter, events));

    const auto &histogram = runner.Metrics().requested_pacing_delays;
    const auto &buckets = histogram.Buckets();

    ASSERT_EQ(histogram.Count(), 4U);
    EXPECT_EQ(histogram.Total(), std::chrono::nanoseconds{2105500});
    EXPECT_EQ(histogram.Min(), std::chrono::nanoseconds{500});
    EXPECT_EQ(histogram.Max(), std::chrono::nanoseconds{2000000});
    EXPECT_EQ(histogram.OverflowCount(), 0U);

    ASSERT_EQ(buckets.size(), 7U);
    EXPECT_EQ(buckets[0].count, 1U);
    EXPECT_EQ(buckets[1].count, 1U);
    EXPECT_EQ(buckets[2].count, 1U);
    EXPECT_EQ(buckets[3].count, 0U);
    EXPECT_EQ(buckets[4].count, 1U);
    EXPECT_EQ(buckets[5].count, 0U);
    EXPECT_EQ(buckets[6].count, 0U);
}

TEST(ReplayRunnerPacingTest, StartOffsetResetsPacingHistogramBaseline) {
    const std::vector<ExternalOrderEvent> events{
        MakeEvent(EventType::New, false, 100.0, 0.01, 1, "open", 0),
        MakeEvent(EventType::New, false, 99.0, 0.01, 1, "open", 100),
        MakeEvent(EventType::New, true, 101.0, 0.01, 1, "open", 1100),
        MakeEvent(EventType::New, true, 102.0, 0.01, 1, "open", 11100),
    };

    ReplayConfig config = MakeQuietReplayConfig();
    config.start_offset = 1;
    config.pacing_mode = ReplayPacingMode::EventTime;
    config.replay_speed = 1.0;

    RecordingReplayClock clock;
    RecordingReplayAdapter adapter;
    ReplayRunner runner(config, clock);

    ASSERT_TRUE(runner.Run(adapter, events));

    const auto &histogram = runner.Metrics().requested_pacing_delays;

    ASSERT_EQ(histogram.Count(), 2U);
    EXPECT_EQ(histogram.Total(), std::chrono::nanoseconds{11000});
    EXPECT_EQ(histogram.Min(), std::chrono::nanoseconds{1000});
    EXPECT_EQ(histogram.Max(), std::chrono::nanoseconds{10000});
}

} // namespace bookforge