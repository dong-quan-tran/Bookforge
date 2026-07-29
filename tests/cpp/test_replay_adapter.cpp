#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "IReplayAdapter.hpp"
#include "core/matching_engine.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

namespace bookforge {
namespace {

ExternalOrderEvent MakeEvent(EventType type, double price = 100.0, double size = 1.0,
                             bool isAsk = false, int statusId = 0,
                             std::string statusText = "test") {
    ExternalOrderEvent ev{};
    ev.ts = std::chrono::nanoseconds{1};
    ev.price = price;
    ev.size = size;
    ev.isAsk = isAsk;
    ev.statusId = statusId;
    ev.statusText = std::move(statusText);
    ev.eventType = type;
    return ev;
}

class StubReplayAdapter final : public IReplayAdapter {
  public:
    void OnEvent(const ExternalOrderEvent &ev) override {
        seen.push_back(ev.eventType);
        switch (ev.eventType) {
        case EventType::New:
            ++metrics_.submitted;
            ++metrics_.newEvents;
            break;
        case EventType::Cancel:
            ++metrics_.unsupported;
            ++metrics_.cancelEvents;
            break;
        case EventType::Fill:
            ++metrics_.unsupported;
            ++metrics_.fillEvents;
            break;
        case EventType::Reject:
            ++metrics_.rejected;
            ++metrics_.rejectEvents;
            break;
        case EventType::Trigger:
            ++metrics_.ignored;
            ++metrics_.triggerEvents;
            break;
        case EventType::Other:
            ++metrics_.ignored;
            ++metrics_.otherEvents;
            break;
        }
    }

    void OnInjectedOrder(const InjectedOrder &) override { ++injected_calls; }

    const AdapterMetrics &Metrics() const override { return metrics_; }

    std::vector<EventType> seen;
    std::size_t injected_calls{0};

  private:
    AdapterMetrics metrics_{};
};

TEST(HyperliquidMatchingEngineAdapterTest, NewEventSubmitsPassiveOrder) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, 100.0, 2.0, false, 1, "open"));

    EXPECT_EQ(adapter.Metrics().submitted, 1u);
    EXPECT_EQ(adapter.Metrics().newEvents, 1u);
    EXPECT_EQ(adapter.Stats().submittedOrders, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, ZeroSizeNewEventIsIgnored) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::New, 100.0, 0.0, false, 1, "open"));

    EXPECT_EQ(adapter.Metrics().submitted, 0u);
    EXPECT_EQ(adapter.Metrics().ignored, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, CancelIsUnsupportedWithoutIdLinkage) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Cancel, 100.0, 1.0, false, 2, "canceled"));

    EXPECT_EQ(adapter.Metrics().unsupported, 1u);
    EXPECT_EQ(adapter.Metrics().cancelEvents, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, FillIsUnsupportedForNow) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Fill, 100.0, 1.0, false, 3, "filled"));

    EXPECT_EQ(adapter.Metrics().unsupported, 1u);
    EXPECT_EQ(adapter.Metrics().fillEvents, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, RejectCountsAsRejected) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Reject, 100.0, 1.0, false, 4, "perpMarginRejected"));

    EXPECT_EQ(adapter.Metrics().rejected, 1u);
    EXPECT_EQ(adapter.Metrics().rejectEvents, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, TriggerCountsAsIgnored) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Trigger, 100.0, 1.0, false, 5, "trigger"));

    EXPECT_EQ(adapter.Metrics().ignored, 1u);
    EXPECT_EQ(adapter.Metrics().triggerEvents, 1u);
}

TEST(HyperliquidMatchingEngineAdapterTest, OtherCountsAsIgnored) {
    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    adapter.OnEvent(MakeEvent(EventType::Other, 100.0, 1.0, false, 6, "other"));

    EXPECT_EQ(adapter.Metrics().ignored, 1u);
    EXPECT_EQ(adapter.Metrics().otherEvents, 1u);
}

TEST(ReplayRunnerTest, StubBackedReplayIntegrationRespectsOffsetAndLimit) {
    ReplayConfig config;
    config.start_offset = 1;
    config.max_events = 3;
    config.log_summary = false;
    config.log_every_n = 0;

    ReplayRunner runner(config);
    StubReplayAdapter adapter;

    std::vector<ExternalOrderEvent> events{MakeEvent(EventType::New), MakeEvent(EventType::Cancel),
                                           MakeEvent(EventType::Fill), MakeEvent(EventType::Reject),
                                           MakeEvent(EventType::Other)};

    const bool ok = runner.Run(adapter, events);

    ASSERT_TRUE(ok);
    ASSERT_EQ(adapter.seen.size(), 3u);
    EXPECT_EQ(adapter.seen[0], EventType::Cancel);
    EXPECT_EQ(adapter.seen[1], EventType::Fill);
    EXPECT_EQ(adapter.seen[2], EventType::Reject);
    EXPECT_EQ(adapter.Metrics().unsupported, 2u);
    EXPECT_EQ(adapter.Metrics().rejected, 1u);
    EXPECT_EQ(adapter.injected_calls, 0u);
}

} // namespace
} // namespace bookforge